/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/AdbKeyState.h"

#include "deskflow/KeyTypes.h"

#include <QTest>

class AdbKeyStateTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void mapsCommonKeys()
  {
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode('a'), 29);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode('Z'), 54);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode('0'), 7);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode('!'), 8);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode('>'), 56);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyReturn), 66);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyLeft), 21);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyF12), 142);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyAudioUp), 24);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyCapsLock), 115);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyNumLock), 143);
    QCOMPARE(deskflow::AdbKeyState::androidKeyCode(kKeyScrollLock), 116);
  }

  void mapsModifiers()
  {
    const auto mask = KeyModifierShift | KeyModifierControl | KeyModifierSuper | KeyModifierCapsLock;
    const int expected = 0x00000001 | 0x00001000 | 0x00010000 | 0x00100000;
    QCOMPARE(deskflow::AdbKeyState::androidMetaState(mask), expected);
  }

  void mapsSideSpecificModifiers()
  {
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyShift_L), 0x00000001 | 0x00000040);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyShift_R), 0x00000001 | 0x00000080);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyAlt_L), 0x00000002 | 0x00000010);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyAlt_R), 0x00000002 | 0x00000020);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyControl_L), 0x00001000 | 0x00002000);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeyControl_R), 0x00001000 | 0x00004000);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeySuper_L), 0x00010000 | 0x00020000);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState(kKeySuper_R), 0x00010000 | 0x00040000);
    QCOMPARE(deskflow::AdbKeyState::androidModifierMetaState('k'), 0);
  }
};

QTEST_MAIN(AdbKeyStateTests)

#include "AdbKeyStateTests.moc"
