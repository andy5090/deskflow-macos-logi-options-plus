/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class OSXScreenTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void emergencyReturnKey_acceptsRequiredChord();
  void emergencyReturnKey_acceptsAdditionalModifiers();
  void emergencyReturnKey_rejectsIncompleteOrRepeatedChord();
  void enforceAsciiInputSource_onlyWhileControllingRemote();
  void remoteCapsLockMask_togglesOnlyForPhysicalKey();
  void navigationGesturesEnabledFromOptions_enabledOption_returnsTrue();
  void navigationGesturesEnabledFromOptions_disabledOption_returnsFalse();
  void navigationGesturesEnabledFromOptions_unrelatedOption_preservesCurrentValue();
  void navigationGesturesEnabledFromOptions_malformedOptions_preservesCurrentValue();
  void classifyNavigationGesture_swipeLeft_returnsLeft();
  void classifyNavigationGesture_swipeRight_returnsRight();
  void classifyNavigationGesture_swipeUp_returnsUp();
  void classifyNavigationGesture_swipeDown_returnsDown();
  void classifyNavigationGesture_ambiguousDelta_returnsNone();
  void classifyNavigationGesture_disabled_returnsNone();
  void classifyNavigationGesture_localScreen_returnsNone();
  void classifyNavigationGesture_nonGestureEvent_returnsNone();
  void classifyNavigationGesture_zeroDelta_returnsNone();
  void navigationActionSlotForDirection_configuredDirections_returnsSlots();
  void navigationDirectionsFromOptions_configuredOptions_returnsDirections();
};
