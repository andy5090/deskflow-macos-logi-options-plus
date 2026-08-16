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

QTEST_MAIN(OSXScreenTests)
