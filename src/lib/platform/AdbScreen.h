/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/Clipboard.h"
#include "deskflow/PlatformScreen.h"
#include "platform/AdbInputBridge.h"
#include "platform/AdbKeyState.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>

namespace deskflow {

class AdbScreen : public PlatformScreen
{
public:
  explicit AdbScreen(IEventQueue *events);
  ~AdbScreen() override;

  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *clipboard) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  void reconfigure(uint32_t activeSides) override;
  uint32_t activeSides() override;
  void warpCursor(int32_t x, int32_t y) override;
  uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override;
  void unregisterHotKey(uint32_t id) override;
  void fakeInputBegin() override;
  void fakeInputEnd() override;
  int32_t getJumpZoneSize() const override;
  bool isAnyMouseButtonDown(uint32_t &buttonID) const override;
  void getCursorCenter(int32_t &x, int32_t &y) const override;

  void fakeMouseButton(ButtonID id, bool press) override;
  void fakeMouseMove(int32_t x, int32_t y) override;
  void fakeMouseRelativeMove(int32_t dx, int32_t dy) const override;
  bool requiresRelativeMouseMoves() const override;
  void fakeMouseWheel(ScrollDelta delta) const override;
  bool fakeMediaKey(KeyID id) override;

  void enable() override;
  void disable() override;
  void enter() override;
  bool canLeave() override;
  void leave() override;
  bool setClipboard(ClipboardID id, const IClipboard *clipboard) override;
  void checkClipboards() override;
  void openScreensaver(bool notify) override;
  void closeScreensaver() override;
  void screensaver(bool activate) override;
  void resetOptions() override;
  void setOptions(const OptionsList &options) override;
  void setSequenceNumber(uint32_t sequenceNumber) override;
  bool isPrimary() const override;
  std::string getSecureInputApp() const override;

protected:
  void handleSystemEvent(const Event &event) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;

private:
  static int androidButton(ButtonID id);
  void releaseAllButtons();
  void sendClipboardEvent(EventTypes type, ClipboardID id);
  int32_t clampedX(int32_t x) const;
  int32_t clampedY(int32_t y) const;

  IEventQueue *m_events;
  mutable AdbInputBridge m_bridge;
  AdbKeyState m_keyState;
  mutable std::array<Clipboard, kClipboardEnd> m_clipboards;
  mutable int32_t m_cursorX = 0;
  mutable int32_t m_cursorY = 0;
  mutable bool m_suppressNextAbsoluteMove = false;
  uint32_t m_activeSides = 0;
  uint32_t m_sequenceNumber = 0;
  IClipboard::Time m_clipboardTime = 0;
  std::string m_lastAndroidClipboard;
  std::set<ButtonID> m_pressedButtons;
  bool m_androidClipboardInitialized = false;
  bool m_enabled = false;
  bool m_isOnScreen = false;
};

} // namespace deskflow
