/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/AdbInputBridge.h"

#include "base/Log.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>

#ifndef DESKFLOW_ADB_BRIDGE_PATH
#define DESKFLOW_ADB_BRIDGE_PATH ""
#endif

namespace deskflow {

AdbInputBridge::AdbInputBridge()
    : m_adb{qEnvironmentVariable("DESKFLOW_ADB", QStringLiteral("adb"))},
      m_serial{qEnvironmentVariable("DESKFLOW_ADB_SERIAL")},
      m_mouseMode{qEnvironmentVariable("DESKFLOW_ANDROID_MOUSE_MODE", QStringLiteral("uhid")).trimmed().toLower()},
      m_keyboardMode{qEnvironmentVariable("DESKFLOW_ANDROID_KEYBOARD_MODE", QStringLiteral("uhid")).trimmed().toLower()}
{
  if (m_mouseMode != QStringLiteral("uhid") && m_mouseMode != QStringLiteral("sdk")) {
    LOG_WARN("unknown Android mouse mode '%s'; using uhid", qPrintable(m_mouseMode));
    m_mouseMode = QStringLiteral("uhid");
  }
  if (m_keyboardMode != QStringLiteral("uhid") && m_keyboardMode != QStringLiteral("sdk")) {
    LOG_WARN("unknown Android keyboard mode '%s'; using uhid", qPrintable(m_keyboardMode));
    m_keyboardMode = QStringLiteral("uhid");
  }
  const QString configuredScale = qEnvironmentVariable("DESKFLOW_ANDROID_MOUSE_SCALE").trimmed();
  if (!configuredScale.isEmpty()) {
    bool ok = false;
    const double scale = configuredScale.toDouble(&ok);
    if (ok && std::isfinite(scale) && scale >= 0.1 && scale <= 10.0) {
      m_mouseScale = scale;
    } else {
      LOG_WARN("invalid Android mouse scale '%s'; using 1.0", qPrintable(configuredScale));
    }
  }
  bool ok = false;
  const int configuredDisplay = qEnvironmentVariableIntValue("DESKFLOW_ANDROID_DISPLAY_ID", &ok);
  if (ok && configuredDisplay >= 0) {
    m_displayId = configuredDisplay;
  }
}

AdbInputBridge::~AdbInputBridge()
{
  stop();
}

QStringList AdbInputBridge::adbArguments(const QStringList &arguments) const
{
  QStringList result;
  if (!m_serial.isEmpty()) {
    result << QStringLiteral("-s") << m_serial;
  }
  result << arguments;
  return result;
}

bool AdbInputBridge::runAdb(const QStringList &arguments, QByteArray *standardOutput) const
{
  QProcess process;
  process.start(m_adb, adbArguments(arguments));
  if (!process.waitForStarted(5000) || !process.waitForFinished(10000) || process.exitCode() != 0) {
    LOG_WARN("adb command failed: %s", qPrintable(QString::fromUtf8(process.readAllStandardError()).trimmed()));
    return false;
  }
  if (standardOutput != nullptr) {
    *standardOutput = process.readAllStandardOutput();
  }
  return true;
}

QString AdbInputBridge::bridgePath() const
{
  const QString configured = qEnvironmentVariable("DESKFLOW_ADB_BRIDGE");
  if (!configured.isEmpty()) {
    return configured;
  }

  const QString buildPath = QString::fromUtf8(DESKFLOW_ADB_BRIDGE_PATH);
  if (QFileInfo::exists(buildPath)) {
    return buildPath;
  }

  const QDir binDir{QCoreApplication::applicationDirPath()};
  const QString installed = binDir.filePath(QStringLiteral("../share/deskflow/android/deskflow-input-bridge.dex"));
  if (QFileInfo::exists(installed)) {
    return QFileInfo{installed}.canonicalFilePath();
  }
  return {};
}

bool AdbInputBridge::queryDisplaySize()
{
  QByteArray output;
  if (runAdb(
          {QStringLiteral("shell"), QStringLiteral("wm"), QStringLiteral("size"), QStringLiteral("-d"),
           QString::number(m_displayId)},
          &output
      )) {
    static const QRegularExpression sizePattern{QStringLiteral("(\\d+)x(\\d+)")};
    auto matches = sizePattern.globalMatch(QString::fromUtf8(output));
    bool foundSize = false;
    while (matches.hasNext()) {
      const auto match = matches.next();
      m_width = match.captured(1).toInt();
      m_height = match.captured(2).toInt();
      foundSize = true;
    }
    if (foundSize && m_width > 0 && m_height > 0) {
      return true;
    }
  }

  const QString configured = qEnvironmentVariable("DESKFLOW_ANDROID_SIZE");
  static const QRegularExpression configuredPattern{QStringLiteral("^(\\d+)x(\\d+)$")};
  const auto match = configuredPattern.match(configured);
  if (match.hasMatch()) {
    m_width = match.captured(1).toInt();
    m_height = match.captured(2).toInt();
    return m_width > 0 && m_height > 0;
  }

  LOG_WARN("could not query Android display size; using 1920x1080 (override with DESKFLOW_ANDROID_SIZE)");
  return true;
}

bool AdbInputBridge::start()
{
  QByteArray state;
  if (!runAdb({QStringLiteral("get-state")}, &state) || state.trimmed() != QByteArrayLiteral("device")) {
    LOG_WARN("no authorized adb device is connected");
    return false;
  }

  const QString localBridge = bridgePath();
  if (localBridge.isEmpty()) {
    LOG_ERR("Deskflow Android input bridge dex was not found");
    return false;
  }

  static const QString remoteBridge = QStringLiteral("/data/local/tmp/deskflow-input-bridge.dex");
  if (!runAdb({QStringLiteral("push"), localBridge, remoteBridge})) {
    return false;
  }
  queryDisplaySize();

  const QStringList command = adbArguments(
      {QStringLiteral("shell"), QStringLiteral("CLASSPATH=%1").arg(remoteBridge), QStringLiteral("app_process"),
       QStringLiteral("/system/bin"), QStringLiteral("org.deskflow.android.InputBridge"), QString::number(m_displayId),
       m_mouseMode, m_keyboardMode}
  );
  // Keep stdout private for the READY handshake, but forward bridge errors so
  // a repeated injection failure cannot fill an unread stderr pipe.
  m_process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
  m_process.start(m_adb, command);
  if (!m_process.waitForStarted(5000) || !m_process.waitForReadyRead(5000)) {
    LOG_ERR("failed to start Android input bridge: %s", m_process.readAllStandardError().constData());
    stop();
    return false;
  }

  const QByteArray greeting = m_process.readLine().trimmed();
  if (!greeting.startsWith(QByteArrayLiteral("READY "))) {
    LOG_ERR(
        "Android input bridge did not become ready: %s %s", greeting.constData(),
        m_process.readAllStandardError().constData()
    );
    stop();
    return false;
  }
  m_relativeMouse = greeting.contains(QByteArrayLiteral("MOUSE_UHID"));
  m_uhidKeyboard = greeting.contains(QByteArrayLiteral("KEYBOARD_UHID"));
  if (m_mouseMode == QStringLiteral("uhid") && !m_relativeMouse) {
    LOG_WARN("Android UHID mouse was unavailable; falling back to absolute SDK events");
  }
  if (m_keyboardMode == QStringLiteral("uhid") && !m_uhidKeyboard) {
    LOG_WARN("Android UHID keyboard was unavailable; falling back to SDK key events");
  }

  LOG_INFO(
      "Android ADB input bridge ready (display=%d, size=%dx%d, mouse=%s, mouse-scale=%.2f, keyboard=%s)", m_displayId,
      static_cast<int>(m_width), static_cast<int>(m_height), m_relativeMouse ? "uhid" : "sdk", m_mouseScale,
      m_uhidKeyboard ? "uhid" : "sdk"
  );
  return true;
}

void AdbInputBridge::stop()
{
  if (m_process.state() == QProcess::NotRunning) {
    return;
  }
  sendLine(QByteArrayLiteral("Q\n"));
  m_process.closeWriteChannel();
  if (!m_process.waitForFinished(1000)) {
    m_process.terminate();
    if (!m_process.waitForFinished(1000)) {
      m_process.kill();
      m_process.waitForFinished(1000);
    }
  }
}

void AdbInputBridge::sendLine(const QByteArray &line) const
{
  if (m_process.state() != QProcess::Running) {
    return;
  }
  if (m_process.write(line) != line.size()) {
    LOG_WARN("failed to queue Android input bridge command");
    return;
  }

  // Deskflow core uses its own event queue, so a queued QProcess write may
  // otherwise wait forever for a Qt event dispatcher that is not running.
  if (m_process.bytesToWrite() > 0 && !m_process.waitForBytesWritten(100)) {
    LOG_WARN("timed out writing Android input bridge command");
  }
}

void AdbInputBridge::sendKey(bool down, int keyCode, int metaState, int repeat) const
{
  sendLine(
      QByteArrayLiteral("K ") + QByteArray::number(down ? 0 : 1) + ' ' + QByteArray::number(keyCode) + ' ' +
      QByteArray::number(metaState) + ' ' + QByteArray::number(repeat) + '\n'
  );
}

void AdbInputBridge::sendMouseMove(int32_t x, int32_t y) const
{
  sendLine(QByteArrayLiteral("M ") + QByteArray::number(x) + ' ' + QByteArray::number(y) + '\n');
}

void AdbInputBridge::sendMouseRelativeMove(int32_t dx, int32_t dy) const
{
  const auto scaledDelta = [this](int32_t delta, double &remainder) {
    if (delta == 0) {
      return int32_t{0};
    }
    const double value = static_cast<double>(delta) * m_mouseScale + remainder;
    const auto scaled = static_cast<int32_t>(value);
    remainder = value - static_cast<double>(scaled);
    return scaled;
  };
  const int32_t scaledX = scaledDelta(dx, m_mouseRemainderX);
  const int32_t scaledY = scaledDelta(dy, m_mouseRemainderY);
  if (scaledX != 0 || scaledY != 0) {
    sendLine(QByteArrayLiteral("R ") + QByteArray::number(scaledX) + ' ' + QByteArray::number(scaledY) + '\n');
  }
}

bool AdbInputBridge::queryCursorPosition(int32_t &x, int32_t &y) const
{
  QByteArray output;
  if (!runAdb({QStringLiteral("shell"), QStringLiteral("dumpsys"), QStringLiteral("SurfaceFlinger")}, &output)) {
    return false;
  }

  // Samsung DeX exposes the hardware cursor rectangle in the SurfaceFlinger
  // layer table. Its top-left corner is the pointer hot spot on current One UI
  // releases. Keep this best-effort: other Android vendors may omit the row.
  static const QRegularExpression cursorPattern{
      QStringLiteral("\\|\\s*CURSOR\\s*\\|[^\\n]*?\\|\\s*(-?\\d+)\\s+(-?\\d+)\\s+-?\\d+\\s+-?\\d+\\s*\\|")
  };
  const auto match = cursorPattern.match(QString::fromUtf8(output));
  if (!match.hasMatch()) {
    return false;
  }

  x = match.captured(1).toInt();
  y = match.captured(2).toInt();
  return true;
}

bool AdbInputBridge::syncMousePosition(int32_t x, int32_t y) const
{
  if (!m_relativeMouse) {
    return false;
  }

  // A UHID mouse is relative, so it retains its previous Android position
  // when Deskflow enters the client at an absolute edge coordinate. Use DeX's
  // cursor layer as feedback. Conservative corrections avoid oscillation when
  // Android applies velocity-dependent pointer acceleration.
  constexpr int maxAttempts = 4;
  constexpr int tolerance = 3;
  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    int32_t currentX = 0;
    int32_t currentY = 0;
    if (!queryCursorPosition(currentX, currentY)) {
      return false;
    }

    const int32_t errorX = x - currentX;
    const int32_t errorY = y - currentY;
    if (std::abs(errorX) <= tolerance && std::abs(errorY) <= tolerance) {
      return true;
    }

    const auto correction = [](int32_t error) {
      if (std::abs(error) <= tolerance) {
        return int32_t{0};
      }
      const auto value = static_cast<int32_t>(std::lround(static_cast<double>(error) * 0.3));
      return value == 0 ? (error < 0 ? int32_t{-1} : int32_t{1}) : value;
    };
    sendMouseRelativeMove(correction(errorX), correction(errorY));
  }
  return true;
}

void AdbInputBridge::sendMouseButton(bool down, int button, int32_t x, int32_t y) const
{
  sendLine(
      QByteArrayLiteral("B ") + QByteArray::number(down ? 1 : 0) + ' ' + QByteArray::number(button) + ' ' +
      QByteArray::number(x) + ' ' + QByteArray::number(y) + '\n'
  );
}

void AdbInputBridge::sendMouseWheel(float horizontal, float vertical, int32_t x, int32_t y) const
{
  sendLine(
      QByteArrayLiteral("S ") + QByteArray::number(horizontal) + ' ' + QByteArray::number(vertical) + ' ' +
      QByteArray::number(x) + ' ' + QByteArray::number(y) + '\n'
  );
}

int32_t AdbInputBridge::width() const
{
  return m_width;
}

int32_t AdbInputBridge::height() const
{
  return m_height;
}

int AdbInputBridge::displayId() const
{
  return m_displayId;
}

bool AdbInputBridge::relativeMouse() const
{
  return m_relativeMouse;
}

bool AdbInputBridge::uhidKeyboard() const
{
  return m_uhidKeyboard;
}

} // namespace deskflow
