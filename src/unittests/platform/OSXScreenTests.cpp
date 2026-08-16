/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXScreenTests.h"

#include "platform/OSXScreen.h"

namespace {

constexpr auto kEmergencyModifiers = kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand;

} // namespace

void OSXScreenTests::emergencyReturnKey_acceptsRequiredChord()
{
  QVERIFY(OSXScreen::isEmergencyReturnKey(kCGEventKeyDown, kVK_Escape, kEmergencyModifiers, false));
}

void OSXScreenTests::emergencyReturnKey_acceptsAdditionalModifiers()
{
  QVERIFY(
      OSXScreen::isEmergencyReturnKey(kCGEventKeyDown, kVK_Escape, kEmergencyModifiers | kCGEventFlagMaskShift, false)
  );
}

void OSXScreenTests::emergencyReturnKey_rejectsIncompleteOrRepeatedChord()
{
  QVERIFY(!OSXScreen::isEmergencyReturnKey(kCGEventKeyUp, kVK_Escape, kEmergencyModifiers, false));
  QVERIFY(!OSXScreen::isEmergencyReturnKey(kCGEventKeyDown, kVK_Escape, kEmergencyModifiers, true));
  QVERIFY(!OSXScreen::isEmergencyReturnKey(
      kCGEventKeyDown, kVK_Escape, kCGEventFlagMaskControl | kCGEventFlagMaskCommand, false
  ));
  QVERIFY(!OSXScreen::isEmergencyReturnKey(kCGEventKeyDown, kVK_Return, kEmergencyModifiers, false));
}

void OSXScreenTests::enforceAsciiInputSource_onlyWhileControllingRemote()
{
  QVERIFY(OSXScreen::shouldEnforceAsciiInputSource(true, false, true));
  QVERIFY(!OSXScreen::shouldEnforceAsciiInputSource(true, true, true));
  QVERIFY(!OSXScreen::shouldEnforceAsciiInputSource(true, false, false));
  QVERIFY(!OSXScreen::shouldEnforceAsciiInputSource(false, false, true));
}

void OSXScreenTests::remoteCapsLockMask_togglesOnlyForPhysicalKey()
{
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(0, KeyModifierCapsLock, kVK_CapsLock), KeyModifierCapsLock
  );
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, KeyModifierCapsLock, kVK_CapsLock),
      static_cast<KeyModifierMask>(0)
  );
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, 0, 0xff), KeyModifierCapsLock
  );
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, 0, kVK_ANSI_Period), KeyModifierCapsLock
  );
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, KeyModifierShift, 0xff),
      static_cast<KeyModifierMask>(KeyModifierCapsLock | KeyModifierShift)
  );
}

QTEST_MAIN(OSXScreenTests)
