/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/AdbScreen.h"

#include "base/Event.h"
#include "base/Log.h"
#include "deskflow/IClipboard.h"
#include "deskflow/ScreenException.h"

#include <algorithm>
#include <cmath>

namespace deskflow {

AdbScreen::AdbScreen(IEventQueue *events)
    : PlatformScreen{events},
      m_events{events},
      m_bridge{},
      m_keyState{m_bridge, events}
{
  if (!m_bridge.start()) {
    throw ScreenUnavailableException{};
  }
  m_cursorX = m_bridge.width() / 2;
  m_cursorY = m_bridge.height() / 2;
}

AdbScreen::~AdbScreen()
{
  releaseAllButtons();
  m_keyState.fakeAllKeysUp();
}

void *AdbScreen::getEventTarget() const
{
  return const_cast<AdbScreen *>(this);
}

bool AdbScreen::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  if (id >= kClipboardEnd || clipboard == nullptr) {
    return false;
  }
  return IClipboard::copy(clipboard, &m_clipboards[id]);
}

bool AdbScreen::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  if (id >= kClipboardEnd || clipboard == nullptr) {
    return false;
  }
  // The ADB backend currently keeps the protocol clipboard in memory. A
  // future bridge protocol revision can publish text through ClipboardService.
  return IClipboard::copy(&m_clipboards[id], clipboard);
}

void AdbScreen::getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const
{
  x = 0;
  y = 0;
  width = m_bridge.width();
  height = m_bridge.height();
}

void AdbScreen::getCursorPos(int32_t &x, int32_t &y) const
{
  x = m_cursorX;
  y = m_cursorY;
}

void AdbScreen::reconfigure(uint32_t activeSides)
{
  m_activeSides = activeSides;
}

uint32_t AdbScreen::activeSides()
{
  return m_activeSides;
}

void AdbScreen::warpCursor(int32_t x, int32_t y)
{
  fakeMouseMove(x, y);
}

uint32_t AdbScreen::registerHotKey(KeyID, KeyModifierMask)
{
  return 0;
}

void AdbScreen::unregisterHotKey(uint32_t)
{
}

void AdbScreen::fakeInputBegin()
{
}

void AdbScreen::fakeInputEnd()
{
}

int32_t AdbScreen::getJumpZoneSize() const
{
  return 0;
}

bool AdbScreen::isAnyMouseButtonDown(uint32_t &buttonID) const
{
  if (m_pressedButtons.empty()) {
    return false;
  }
  buttonID = *m_pressedButtons.begin();
  return true;
}

void AdbScreen::getCursorCenter(int32_t &x, int32_t &y) const
{
  x = m_bridge.width() / 2;
  y = m_bridge.height() / 2;
}

int AdbScreen::androidButton(ButtonID id)
{
  switch (id) {
  case kButtonLeft:
    return 1; // BUTTON_PRIMARY
  case kButtonRight:
    return 2; // BUTTON_SECONDARY
  case kButtonMiddle:
    return 4; // BUTTON_TERTIARY
  case kButtonExtra0:
    return 8; // BUTTON_BACK
  case kButtonExtra1:
    return 16; // BUTTON_FORWARD
  default:
    return 0;
  }
}

int32_t AdbScreen::clampedX(int32_t x) const
{
  return std::clamp<int32_t>(x, 0, std::max<int32_t>(0, m_bridge.width() - 1));
}

int32_t AdbScreen::clampedY(int32_t y) const
{
  return std::clamp<int32_t>(y, 0, std::max<int32_t>(0, m_bridge.height() - 1));
}

void AdbScreen::fakeMouseButton(ButtonID id, bool press)
{
  const int button = androidButton(id);
  if (button == 0) {
    return;
  }
  if (press) {
    if (m_pressedButtons.contains(id)) {
      return;
    }
    m_pressedButtons.insert(id);
  } else {
    if (!m_pressedButtons.contains(id)) {
      return;
    }
    m_pressedButtons.erase(id);
  }
  m_bridge.sendMouseButton(press, button, m_cursorX, m_cursorY);
}

void AdbScreen::releaseAllButtons()
{
  while (!m_pressedButtons.empty()) {
    fakeMouseButton(*m_pressedButtons.begin(), false);
  }
}

void AdbScreen::fakeMouseMove(int32_t x, int32_t y)
{
  const int32_t nextX = clampedX(x);
  const int32_t nextY = clampedY(y);
  const int32_t dx = nextX - m_cursorX;
  const int32_t dy = nextY - m_cursorY;
  m_cursorX = nextX;
  m_cursorY = nextY;

  if (m_bridge.relativeMouse()) {
    if (m_suppressNextAbsoluteMove) {
      m_suppressNextAbsoluteMove = false;
      return;
    }
    m_bridge.sendMouseRelativeMove(dx, dy);
  } else {
    m_bridge.sendMouseMove(m_cursorX, m_cursorY);
  }
}

void AdbScreen::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  const int32_t previousX = m_cursorX;
  const int32_t previousY = m_cursorY;
  m_cursorX = clampedX(previousX + dx);
  m_cursorY = clampedY(previousY + dy);
  if (m_bridge.relativeMouse()) {
    m_bridge.sendMouseRelativeMove(m_cursorX - previousX, m_cursorY - previousY);
  } else {
    m_bridge.sendMouseMove(m_cursorX, m_cursorY);
  }
}

void AdbScreen::fakeMouseWheel(ScrollDelta delta) const
{
  delta = applyScrollModifier(delta);
  if (delta.x == 0 && delta.y == 0) {
    return;
  }
  m_bridge.sendMouseWheel(
      static_cast<float>(delta.x) / static_cast<float>(s_scrollDelta),
      static_cast<float>(delta.y) / static_cast<float>(s_scrollDelta), m_cursorX, m_cursorY
  );
}

bool AdbScreen::fakeMediaKey(KeyID id)
{
  return m_keyState.fakeMediaKey(id);
}

void AdbScreen::enable()
{
  m_enabled = true;
}

void AdbScreen::disable()
{
  releaseAllButtons();
  m_keyState.fakeAllKeysUp();
  m_enabled = false;
}

void AdbScreen::enter()
{
  m_isOnScreen = true;
  m_suppressNextAbsoluteMove = m_bridge.relativeMouse();
}

bool AdbScreen::canLeave()
{
  return true;
}

void AdbScreen::leave()
{
  releaseAllButtons();
  m_keyState.fakeAllKeysUp();
  m_isOnScreen = false;
  m_suppressNextAbsoluteMove = m_bridge.relativeMouse();
}

void AdbScreen::checkClipboards()
{
}

void AdbScreen::openScreensaver(bool)
{
}

void AdbScreen::closeScreensaver()
{
}

void AdbScreen::screensaver(bool)
{
}

void AdbScreen::resetOptions()
{
}

void AdbScreen::setOptions(const OptionsList &)
{
}

void AdbScreen::setSequenceNumber(uint32_t sequenceNumber)
{
  m_sequenceNumber = sequenceNumber;
}

bool AdbScreen::isPrimary() const
{
  return false;
}

std::string AdbScreen::getSecureInputApp() const
{
  return {};
}

void AdbScreen::handleSystemEvent(const Event &)
{
}

void AdbScreen::updateButtons()
{
}

IKeyState *AdbScreen::getKeyState() const
{
  return const_cast<AdbKeyState *>(&m_keyState);
}

} // namespace deskflow
