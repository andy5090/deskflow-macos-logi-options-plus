/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/AdbKeyState.h"

#include "base/Log.h"
#include "deskflow/KeyTypes.h"
#include "platform/AdbInputBridge.h"

#include <algorithm>

namespace deskflow {

namespace {

bool needsShift(KeyID id)
{
  return (id >= 'A' && id <= 'Z') || id == '~' || id == '_' || id == '+' || id == '{' || id == '}' || id == '|' ||
         id == ':' || id == '"' || id == '?' || id == '<' || id == '>' || id == ')' || id == '!' || id == '#' ||
         id == '$' || id == '%' || id == '^' || id == '&' || id == '*' || id == '(';
}

KeyModifierMask deskflowModifierMask(KeyID id)
{
  switch (id) {
  case kKeyShift_L:
  case kKeyShift_R:
    return KeyModifierShift;
  case kKeyControl_L:
  case kKeyControl_R:
    return KeyModifierControl;
  case kKeyAlt_L:
    return KeyModifierAlt;
  case kKeyAlt_R:
  case kKeyAltGr:
    return KeyModifierAltGr;
  case kKeyMeta_L:
  case kKeyMeta_R:
    return KeyModifierMeta;
  case kKeySuper_L:
  case kKeySuper_R:
    return KeyModifierSuper;
  default:
    return 0;
  }
}

} // namespace

AdbKeyState::AdbKeyState(AdbInputBridge &bridge, IEventQueue *events) : IKeyState{events}, m_bridge{bridge}
{
}

void AdbKeyState::updateKeyMap()
{
}

void AdbKeyState::updateKeyState()
{
}

void AdbKeyState::setHalfDuplexMask(KeyModifierMask mask)
{
  m_halfDuplexMask = mask;
}

int AdbKeyState::androidMetaState(KeyModifierMask mask)
{
  int state = 0;
  if ((mask & KeyModifierShift) != 0) {
    state |= 0x00000001; // META_SHIFT_ON
  }
  if ((mask & KeyModifierAlt) != 0 || (mask & KeyModifierAltGr) != 0) {
    state |= 0x00000002; // META_ALT_ON
  }
  if ((mask & KeyModifierControl) != 0) {
    state |= 0x00001000; // META_CTRL_ON
  }
  if ((mask & (KeyModifierMeta | KeyModifierSuper)) != 0) {
    state |= 0x00010000; // META_META_ON
  }
  if ((mask & KeyModifierCapsLock) != 0) {
    state |= 0x00100000; // META_CAPS_LOCK_ON
  }
  if ((mask & KeyModifierNumLock) != 0) {
    state |= 0x00200000; // META_NUM_LOCK_ON
  }
  if ((mask & KeyModifierScrollLock) != 0) {
    state |= 0x00400000; // META_SCROLL_LOCK_ON
  }
  return state;
}

int AdbKeyState::androidModifierMetaState(KeyID id)
{
  switch (id) {
  case kKeyShift_L:
    return 0x00000001 | 0x00000040; // META_SHIFT_ON | META_SHIFT_LEFT_ON
  case kKeyShift_R:
    return 0x00000001 | 0x00000080; // META_SHIFT_ON | META_SHIFT_RIGHT_ON
  case kKeyAlt_L:
    return 0x00000002 | 0x00000010; // META_ALT_ON | META_ALT_LEFT_ON
  case kKeyAlt_R:
  case kKeyAltGr:
    return 0x00000002 | 0x00000020; // META_ALT_ON | META_ALT_RIGHT_ON
  case kKeyControl_L:
    return 0x00001000 | 0x00002000; // META_CTRL_ON | META_CTRL_LEFT_ON
  case kKeyControl_R:
    return 0x00001000 | 0x00004000; // META_CTRL_ON | META_CTRL_RIGHT_ON
  case kKeyMeta_L:
  case kKeySuper_L:
    return 0x00010000 | 0x00020000; // META_META_ON | META_META_LEFT_ON
  case kKeyMeta_R:
  case kKeySuper_R:
    return 0x00010000 | 0x00040000; // META_META_ON | META_META_RIGHT_ON
  default:
    return 0;
  }
}

void AdbKeyState::updateAndroidModifierState()
{
  m_androidModifierState = 0;
  m_activeModifiers = 0;
  for (const auto &[button, key] : m_pressed) {
    (void)button;
    m_androidModifierState |= key.modifierMetaState;
    m_activeModifiers |= key.modifierMask;
  }
}

int AdbKeyState::androidKeyCode(KeyID id)
{
  if (id >= 'a' && id <= 'z') {
    return 29 + static_cast<int>(id - 'a');
  }
  if (id >= 'A' && id <= 'Z') {
    return 29 + static_cast<int>(id - 'A');
  }
  if (id >= '0' && id <= '9') {
    return 7 + static_cast<int>(id - '0');
  }
  if (id >= kKeyF1 && id <= kKeyF12) {
    return 131 + static_cast<int>(id - kKeyF1);
  }
  if (id >= kKeyKP_0 && id <= kKeyKP_9) {
    return 144 + static_cast<int>(id - kKeyKP_0);
  }

  switch (id) {
  case ' ':
    return 62;
  case ',':
  case '<':
    return 55;
  case '.':
  case '>':
    return 56;
  case ')':
    return 7;
  case '!':
    return 8;
  case '#':
    return 10;
  case '$':
    return 11;
  case '%':
    return 12;
  case '^':
    return 13;
  case '&':
    return 14;
  case '*':
    return 15;
  case '(':
    return 16;
  case '`':
  case '~':
    return 68;
  case '-':
  case '_':
    return 69;
  case '=':
  case '+':
    return 70;
  case '[':
  case '{':
    return 71;
  case ']':
  case '}':
    return 72;
  case '\\':
  case '|':
    return 73;
  case ';':
  case ':':
    return 74;
  case '\'':
  case '"':
    return 75;
  case '/':
  case '?':
    return 76;
  case '@':
    return 77;
  case kKeyBackSpace:
    return 67;
  case kKeyTab:
  case kKeyLeftTab:
    return 61;
  case kKeyReturn:
  case kKeyLinefeed:
    return 66;
  case kKeyEscape:
    return 111;
  case kKeyDelete:
    return 112;
  case kKeyInsert:
    return 124;
  case kKeyHome:
    return 122;
  case kKeyEnd:
    return 123;
  case kKeyLeft:
    return 21;
  case kKeyRight:
    return 22;
  case kKeyUp:
    return 19;
  case kKeyDown:
    return 20;
  case kKeyPageUp:
    return 92;
  case kKeyPageDown:
    return 93;
  case kKeyMenu:
    return 82;
  case kKeyPrint:
    return 120;
  case kKeyPause:
  case kKeyBreak:
    return 121;
  case kKeyNumLock:
    return 143;
  case kKeyScrollLock:
    return 116;
  case kKeyCapsLock:
    return 115;
  case kKeyShift_L:
    return 59;
  case kKeyShift_R:
    return 60;
  case kKeyControl_L:
    return 113;
  case kKeyControl_R:
    return 114;
  case kKeyAlt_L:
    return 57;
  case kKeyAlt_R:
  case kKeyAltGr:
    return 58;
  case kKeyMeta_L:
  case kKeySuper_L:
    return 117;
  case kKeyMeta_R:
  case kKeySuper_R:
    return 118;
  case kKeyHangul:
    return 204;
  case kKeyKP_Divide:
    return 154;
  case kKeyKP_Multiply:
    return 155;
  case kKeyKP_Subtract:
    return 156;
  case kKeyKP_Add:
    return 157;
  case kKeyKP_Decimal:
    return 158;
  case kKeyKP_Enter:
    return 160;
  case kKeyKP_Equal:
    return 161;
  case kKeyWWWBack:
    return 4;
  case kKeyWWWForward:
    return 125;
  case kKeyWWWSearch:
    return 84;
  case kKeyWWWHome:
    return 3;
  case kKeyAudioMute:
    return 164;
  case kKeyAudioDown:
    return 25;
  case kKeyAudioUp:
    return 24;
  case kKeyAudioNext:
    return 87;
  case kKeyAudioPrev:
    return 88;
  case kKeyAudioStop:
    return 86;
  case kKeyAudioPlay:
    return 85;
  case kKeyBrightnessDown:
    return 220;
  case kKeyBrightnessUp:
    return 221;
  default:
    return 0;
  }
}

void AdbKeyState::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &)
{
  const int keyCode = androidKeyCode(id);
  if (keyCode == 0) {
    LOG_DEBUG("Android ADB backend cannot map key id 0x%x", id);
    return;
  }
  const int modifierMetaState = androidModifierMetaState(id);
  const KeyModifierMask modifierMask = deskflowModifierMask(id);
  int requiredMetaState = androidMetaState(mask) & 0x00700000; // lock state
  if (needsShift(id)) {
    requiredMetaState |= 0x00000001;
  }
  if (const auto found = m_pressed.find(button); found != m_pressed.end() && found->second.keyCode != keyCode) {
    m_bridge.sendKey(false, found->second.keyCode, m_androidModifierState | found->second.requiredMetaState);
    m_pressed.erase(found);
    updateAndroidModifierState();
  }
  const int metaState = modifierMetaState == 0 ? (androidMetaState(mask) | m_androidModifierState | requiredMetaState)
                                               : (m_androidModifierState | requiredMetaState);
  m_pressed[button] = {keyCode, requiredMetaState, modifierMetaState, modifierMask};
  m_bridge.sendKey(true, keyCode, metaState);
  updateAndroidModifierState();
}

bool AdbKeyState::fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &)
{
  const int keyCode = androidKeyCode(id);
  if (keyCode == 0) {
    return false;
  }
  int metaState = androidMetaState(mask) | m_androidModifierState;
  if (needsShift(id)) {
    metaState |= 0x00000001;
  }
  const auto found = m_pressed.find(button);
  const int modifierMetaState =
      found == m_pressed.end() ? androidModifierMetaState(id) : found->second.modifierMetaState;
  const KeyModifierMask modifierMask = found == m_pressed.end() ? deskflowModifierMask(id) : found->second.modifierMask;
  const int requiredMetaState = needsShift(id) ? 0x00000001 : 0;
  m_pressed[button] = {keyCode, requiredMetaState, modifierMetaState, modifierMask};
  m_bridge.sendKey(true, keyCode, metaState, std::max<int32_t>(1, count));
  return true;
}

bool AdbKeyState::fakeKeyUp(KeyButton button)
{
  const auto found = m_pressed.find(button);
  if (found == m_pressed.end()) {
    return false;
  }
  m_bridge.sendKey(false, found->second.keyCode, m_androidModifierState | found->second.requiredMetaState);
  m_pressed.erase(found);
  updateAndroidModifierState();
  return true;
}

void AdbKeyState::fakeAllKeysUp()
{
  while (!m_pressed.empty()) {
    const auto found = std::find_if(m_pressed.begin(), m_pressed.end(), [](const auto &entry) {
      return entry.second.modifierMetaState == 0;
    });
    const auto key = found == m_pressed.end() ? std::prev(m_pressed.end()) : found;
    m_bridge.sendKey(false, key->second.keyCode, m_androidModifierState | key->second.requiredMetaState);
    m_pressed.erase(key);
    updateAndroidModifierState();
  }
}

bool AdbKeyState::fakeCtrlAltDel()
{
  constexpr int ctrl = 113;
  constexpr int alt = 57;
  constexpr int del = 112;
  constexpr int ctrlMeta = 0x00001000 | 0x00002000;
  constexpr int altMeta = 0x00000002 | 0x00000010;
  m_bridge.sendKey(true, ctrl, 0);
  m_bridge.sendKey(true, alt, ctrlMeta);
  m_bridge.sendKey(true, del, ctrlMeta | altMeta);
  m_bridge.sendKey(false, del, ctrlMeta | altMeta);
  m_bridge.sendKey(false, alt, ctrlMeta | altMeta);
  m_bridge.sendKey(false, ctrl, ctrlMeta);
  return true;
}

bool AdbKeyState::fakeMediaKey(KeyID id)
{
  const int keyCode = androidKeyCode(id);
  if (keyCode == 0) {
    return false;
  }
  m_bridge.sendKey(true, keyCode, 0);
  m_bridge.sendKey(false, keyCode, 0);
  return true;
}

bool AdbKeyState::isKeyDown(KeyButton button) const
{
  return m_pressed.contains(button);
}

KeyModifierMask AdbKeyState::getActiveModifiers() const
{
  return m_activeModifiers;
}

KeyModifierMask AdbKeyState::pollActiveModifiers() const
{
  return m_activeModifiers;
}

int32_t AdbKeyState::pollActiveGroup() const
{
  return 0;
}

void AdbKeyState::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  for (const auto &[button, key] : m_pressed) {
    (void)key;
    pressedKeys.insert(button);
  }
}

} // namespace deskflow
