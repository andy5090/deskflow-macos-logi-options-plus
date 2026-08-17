/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QProcess>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace deskflow {

class AdbInputBridge
{
public:
  AdbInputBridge();
  ~AdbInputBridge();

  bool start();
  void stop();

  int32_t width() const;
  int32_t height() const;
  int displayId() const;
  bool relativeMouse() const;
  bool uhidKeyboard() const;

  void sendKey(bool down, int keyCode, int metaState, int repeat = 0) const;
  void sendMouseMove(int32_t x, int32_t y) const;
  void sendMouseRelativeMove(int32_t dx, int32_t dy) const;
  void sendMouseButton(bool down, int button, int32_t x, int32_t y) const;
  void sendMouseWheel(float horizontal, float vertical, int32_t x, int32_t y) const;

private:
  QString bridgePath() const;
  QStringList adbArguments(const QStringList &arguments) const;
  bool runAdb(const QStringList &arguments, QByteArray *standardOutput = nullptr) const;
  bool queryDisplaySize();
  void sendLine(const QByteArray &line) const;

  QString m_adb;
  QString m_serial;
  QString m_mouseMode;
  QString m_keyboardMode;
  int m_displayId = 0;
  int32_t m_width = 1920;
  int32_t m_height = 1080;
  bool m_relativeMouse = false;
  bool m_uhidKeyboard = false;
  mutable QProcess m_process;
};

} // namespace deskflow
