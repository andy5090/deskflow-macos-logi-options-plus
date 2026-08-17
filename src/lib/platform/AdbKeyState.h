/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IKeyState.h"

#include <map>

namespace deskflow {

class AdbInputBridge;

class AdbKeyState : public IKeyState
{
public:
  AdbKeyState(AdbInputBridge &bridge, IEventQueue *events);

  void updateKeyMap() override;
  void updateKeyState() override;
  void setHalfDuplexMask(KeyModifierMask mask) override;
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override;
  bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  bool fakeKeyUp(KeyButton button) override;
  void fakeAllKeysUp() override;
  bool fakeCtrlAltDel() override;
  bool fakeMediaKey(KeyID id) override;
  bool isKeyDown(KeyButton button) const override;
  KeyModifierMask getActiveModifiers() const override;
  KeyModifierMask pollActiveModifiers() const override;
  int32_t pollActiveGroup() const override;
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override;

  static int androidKeyCode(KeyID id);
  static int androidMetaState(KeyModifierMask mask);
  static int androidModifierMetaState(KeyID id);

private:
  struct PressedKey
  {
    int keyCode;
    int requiredMetaState;
    int modifierMetaState;
    KeyModifierMask modifierMask;
  };

  void updateAndroidModifierState();
  void syncLockState(KeyModifierMask mask);
  static KeyModifierMask lockMaskFromAndroidMetaState(int metaState);

  AdbInputBridge &m_bridge;
  std::map<KeyButton, PressedKey> m_pressed;
  KeyModifierMask m_activeModifiers = 0;
  KeyModifierMask m_lockState = 0;
  KeyModifierMask m_halfDuplexMask = 0;
  int m_androidModifierState = 0;
  bool m_lockStateKnown = false;
};

} // namespace deskflow
