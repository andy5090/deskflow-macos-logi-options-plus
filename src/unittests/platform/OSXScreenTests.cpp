/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXScreenTests.h"

#include "platform/OSXScreen.h"

namespace {

constexpr auto kEmergencyModifiers = kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand;

// NSEventTypeGesture is unavailable to this pure C++ test target.
constexpr auto kGestureType = static_cast<CGEventType>(29);

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

void OSXScreenTests::navigationGesturesEnabledFromOptions_enabledOption_returnsTrue()
{
  const OptionsList enabled = {kOptionMacNavigationGestures, 1};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(enabled, false));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_disabledOption_returnsFalse()
{
  const OptionsList disabled = {kOptionMacNavigationGestures, 0};

  QVERIFY(!OSXScreen::navigationGesturesEnabledFromOptions(disabled, true));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_unrelatedOption_preservesCurrentValue()
{
  const OptionsList unrelated = {kOptionClipboardSharing, 1};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(unrelated, true));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_malformedOptions_preservesCurrentValue()
{
  const OptionsList malformed = {kOptionMacNavigationGestures};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(malformed, true));
}

void OSXScreenTests::classifyNavigationGestureButton_swipeLeft_returnsExtra0()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, 1.0), kButtonExtra0);
}

void OSXScreenTests::classifyNavigationGestureButton_swipeRight_returnsExtra1()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, -1.0), kButtonExtra1);
}

void OSXScreenTests::classifyNavigationGestureButton_disabled_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, false, 1.0), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_localScreen_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, true, true, 1.0), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_nonGestureEvent_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kCGEventScrollWheel, false, true, 1.0), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_zeroDelta_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, 0.0), kButtonNone);
}

QTEST_MAIN(OSXScreenTests)
