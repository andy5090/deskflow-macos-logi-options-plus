/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/AdbInputBridge.h"

#include "base/Log.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class AdbInputBridgeTests : public QObject
{
  Q_OBJECT

private:
  Log m_log;

private Q_SLOTS:
  void transfersUtf8ClipboardText()
  {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString adbPath = directory.filePath(QStringLiteral("adb"));
    QFile adb{adbPath};
    QVERIFY(adb.open(QIODevice::WriteOnly));
    const QByteArray script = R"(#!/bin/sh
case "$*" in
  "get-state")
    printf 'device\n'
    ;;
  "push "*)
    ;;
  "shell wm size "*)
    printf 'Physical size: 1920x1080\n'
    ;;
  *" app_process "*)
    clipboard='SGVsbG8g8J+MjQ=='
    printf 'READY 0 MOUSE_UHID KEYBOARD_UHID CLIPBOARD_TEXT\n'
    while IFS= read -r line; do
      set -- $line
      case "$1" in
        CG)
          printf 'CLIP %s OK %s\n' "$2" "$clipboard"
          ;;
        CS)
          clipboard=$3
          printf 'CLIP %s OK -\n' "$2"
          ;;
        Q)
          exit 0
          ;;
      esac
    done
    ;;
  *)
    exit 1
    ;;
esac
)";
    QCOMPARE(adb.write(script), script.size());
    adb.close();
    QVERIFY(QFile::setPermissions(
        adbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
    ));

    const QString bridgePath = directory.filePath(QStringLiteral("bridge.dex"));
    QFile bridge{bridgePath};
    QVERIFY(bridge.open(QIODevice::WriteOnly));
    bridge.close();

    const bool hadAdb = qEnvironmentVariableIsSet("DESKFLOW_ADB");
    const bool hadBridge = qEnvironmentVariableIsSet("DESKFLOW_ADB_BRIDGE");
    const QByteArray oldAdb = qgetenv("DESKFLOW_ADB");
    const QByteArray oldBridge = qgetenv("DESKFLOW_ADB_BRIDGE");
    qputenv("DESKFLOW_ADB", adbPath.toUtf8());
    qputenv("DESKFLOW_ADB_BRIDGE", bridgePath.toUtf8());

    {
      deskflow::AdbInputBridge inputBridge;
      QVERIFY(inputBridge.start());
      QVERIFY(inputBridge.clipboardText());

      std::string text;
      QVERIFY(inputBridge.readClipboardText(text));
      QCOMPARE(text, std::string{"Hello 🌍"});

      const std::string replacement{"Android clipboard\n안드로이드"};
      QVERIFY(inputBridge.writeClipboardText(replacement));
      QVERIFY(inputBridge.readClipboardText(text));
      QCOMPARE(text, replacement);
    }

    if (hadAdb) {
      qputenv("DESKFLOW_ADB", oldAdb);
    } else {
      qunsetenv("DESKFLOW_ADB");
    }
    if (hadBridge) {
      qputenv("DESKFLOW_ADB_BRIDGE", oldBridge);
    } else {
      qunsetenv("DESKFLOW_ADB_BRIDGE");
    }
  }
};

QTEST_GUILESS_MAIN(AdbInputBridgeTests)

#include "AdbInputBridgeTests.moc"
