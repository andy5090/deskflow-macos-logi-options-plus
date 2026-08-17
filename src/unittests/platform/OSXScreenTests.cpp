/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXScreenTests.h"

#include "common/NavigationTypes.h"
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

void OSXScreenTests::classifyNavigationGesture_swipeLeft_returnsLeft()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, 1.0, 0.0),
      NavigationGestureDirection::Left
  );
}

void OSXScreenTests::classifyNavigationGesture_swipeRight_returnsRight()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, -1.0, 0.0),
      NavigationGestureDirection::Right
  );
}

void OSXScreenTests::classifyNavigationGesture_swipeUp_returnsUp()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, 0.0, 1.0), NavigationGestureDirection::Up
  );
}

void OSXScreenTests::classifyNavigationGesture_swipeDown_returnsDown()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, 0.0, -1.0), NavigationGestureDirection::Down
  );
}

void OSXScreenTests::classifyNavigationGesture_ambiguousDelta_returnsNone()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, 1.0, 1.0), NavigationGestureDirection::None
  );
}

void OSXScreenTests::classifyNavigationGesture_disabled_returnsNone()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, false, 1.0, 0.0),
      NavigationGestureDirection::None
  );
}

void OSXScreenTests::classifyNavigationGesture_localScreen_returnsNone()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, true, true, 1.0, 0.0), NavigationGestureDirection::None
  );
}

void OSXScreenTests::classifyNavigationGesture_nonGestureEvent_returnsNone()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kCGEventScrollWheel, false, true, 1.0, 0.0),
      NavigationGestureDirection::None
  );
}

void OSXScreenTests::classifyNavigationGesture_zeroDelta_returnsNone()
{
  QCOMPARE(
      OSXScreen::classifyNavigationGesture(kGestureType, false, true, 0.0, 0.0), NavigationGestureDirection::None
  );
}

void OSXScreenTests::navigationActionSlotForDirection_configuredDirections_returnsSlots()
{
  QCOMPARE(
      OSXScreen::navigationActionSlotForDirection(
          NavigationGestureDirection::Up, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::Action1
  );
  QCOMPARE(
      OSXScreen::navigationActionSlotForDirection(
          NavigationGestureDirection::Down, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::Action2
  );
  QCOMPARE(
      OSXScreen::navigationActionSlotForDirection(
          NavigationGestureDirection::Left, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::None
  );
}

void OSXScreenTests::navigationDirectionsFromOptions_configuredOptions_returnsDirections()
{
  const OptionsList options = {
      kOptionMacNavigationGestureAction1, static_cast<uint32_t>(NavigationGestureDirection::Up),
      kOptionMacNavigationGestureAction2, static_cast<uint32_t>(NavigationGestureDirection::Down)
  };

  auto action1 = NavigationGestureDirection::Left;
  auto action2 = NavigationGestureDirection::Right;
  OSXScreen::navigationDirectionsFromOptions(options, action1, action2);

  QCOMPARE(action1, NavigationGestureDirection::Up);
  QCOMPARE(action2, NavigationGestureDirection::Down);
}

QTEST_MAIN(OSXScreenTests)
