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
};
