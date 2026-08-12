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
  QCOMPARE(OSXScreen::adjustRemoteCapsLockMask(0, KeyModifierCapsLock, kVK_CapsLock), KeyModifierCapsLock);
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, KeyModifierCapsLock, kVK_CapsLock),
      static_cast<KeyModifierMask>(0)
  );
  QCOMPARE(OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, 0, 0xff), KeyModifierCapsLock);
  QCOMPARE(OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, 0, kVK_ANSI_Period), KeyModifierCapsLock);
  QCOMPARE(
      OSXScreen::adjustRemoteCapsLockMask(KeyModifierCapsLock, KeyModifierShift, 0xff),
      static_cast<KeyModifierMask>(KeyModifierCapsLock | KeyModifierShift)
  );
}

void OSXScreenTests::navigationGestureOption_appliesOnlyValidMatchingOptions()
{
  const OptionsList enabled = {kOptionMacNavigationGestures, 1};
  const OptionsList disabled = {kOptionMacNavigationGestures, 0};
  const OptionsList unrelated = {kOptionClipboardSharing, 1};
  const OptionsList malformed = {kOptionMacNavigationGestures};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(enabled, false));
  QVERIFY(!OSXScreen::navigationGesturesEnabledFromOptions(disabled, true));
  QVERIFY(!OSXScreen::navigationGesturesEnabledFromOptions(unrelated, false));
  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(unrelated, true));
  QVERIFY(!OSXScreen::navigationGesturesEnabledFromOptions(malformed, false));
  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(malformed, true));
}

void OSXScreenTests::navigationGesture_forwardsOnlyWhenEnabledAndOffscreen()
{
  // NSEventTypeGesture is unavailable to this pure C++ test target.
  constexpr auto gestureType = static_cast<CGEventType>(29);

  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, false, true, 4), kButtonExtra0);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, false, true, 8), kButtonExtra1);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, false, false, 4), kButtonNone);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, true, true, 4), kButtonNone);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kCGEventScrollWheel, false, true, 4), kButtonNone);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, false, true, 0), kButtonNone);
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(gestureType, false, true, 12), kButtonNone);
}

QTEST_MAIN(OSXScreenTests)
