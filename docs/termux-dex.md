<!--
SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
-->

# Termux and Samsung DeX client (experimental)

This branch can run the Deskflow client natively in Termux and control the
Android or Samsung DeX desktop without installing an Android APK.

## What is and is not possible

The normal Linux X11 backend can control applications inside Termux:X11, but
it cannot inject input into Android applications or the DeX desktop. Android
input injection requires the signature-level `INJECT_EVENTS` permission. The
Android `shell` user has that permission; a regular Termux application UID does
not.

The experimental backend therefore uses this path:

```text
Mac Deskflow server
        | Deskflow protocol over the LAN
Termux deskflow-core client
        | persistent local adb shell
small app_process DEX bridge (shell UID)
        | /dev/uhid virtual keyboard + mouse
        | InputManager fallback for unsupported keys
Android / Samsung DeX display
```

The DEX file is pushed to `/data/local/tmp` and executed directly by
`app_process`. It is not installed as an application, adds no launcher entry,
and stops when the Deskflow client stops.

| Approach | Controls Termux:X11 | Controls Android/DeX | APK | Extra privilege |
|---|---:|---:|---:|---|
| Existing X11 client | Yes | No | No | None |
| This ADB backend | Not its purpose | Yes | No | Wireless debugging / shell |
| Accessibility service | Yes | Partially | Yes | User-enabled service |
| `/dev/uinput` directly | Yes | Yes | No | Root or a vendor policy change |

## Current status

- Termux-native ARM64 `deskflow-core` builds successfully on Android 13.
- The generated DEX loads under Android `app_process` and registers virtual
  UHID keyboard and mouse devices. Android classifies the keyboard as an
  external USB keyboard, so DeX and IME hardware-keyboard shortcuts work.
- End-to-end validation on a Samsung DeX display confirms pointer movement,
  buttons, scrolling, typing, modifiers, and function-key kernel events.
- Common typing keys, modifiers, navigation keys, F1-F12, media keys, Hangul
  language switching, absolute/relative pointer motion, five mouse buttons,
  and horizontal/vertical scrolling are mapped.
- UTF-8 text clipboard sharing works in both directions through the existing
  ADB shell bridge. Server clipboard updates are published to Android
  immediately, and Android clipboard changes are detected when leaving the
  client screen. HTML and bitmap clipboard formats are not supported yet.
- End-to-end behavior can vary with Android/One UI versions and input display
  routing. In particular, the DeX display ID must be selected explicitly.

## Prerequisites

Install Termux and Termux:X11 packages from a mutually compatible source. The
ADB client itself is headless and does not require the Termux:X11 application,
but Qt packages are currently distributed from the Termux X11 repository.

```sh
pkg update
pkg install x11-repo
pkg install git cmake clang make pkg-config openssl android-tools \
  openjdk-17 d8 libxkbcommon qt6-qtbase qt6-qtbase-gtk-platformtheme qt6-qttools
```

Enable **Developer options > Wireless debugging** on the Android device. Pair
Termux with the same device (the pairing port and connection port are
different):

```sh
adb pair localhost:PAIRING_PORT
adb connect localhost:CONNECTION_PORT
adb devices -l
```

The final command must show one device in the `device` state. Pairing is
normally one-time, but the connection port can change and wireless debugging
may need to be re-enabled after a reboot.

## Build

From the repository root:

```sh
./scripts/build-termux-adb.sh
```

The outputs are:

- `build-termux-adb/bin/deskflow-core`
- `build-termux-adb/src/lib/platform/adb-bridge/classes.dex`

The DEX is generated from Java source with Termux `javac` and `d8`; a complete
Android SDK is not required.

## Configure the client

Create `termux-client.conf`:

```ini
[client]
languageSync=false
remoteHost=192.168.1.10

[core]
computerName=dex-termux
port=24800

[security]
tlsEnabled=true
```

Add `dex-termux` as a client screen in the server layout. The ADB client creates
its client certificate automatically when needed. On first connection, verify
and trust the server fingerprint in the same way as other Deskflow clients.

Find the DeX display ID after DeX is active:

```sh
adb shell dumpsys display | grep -E 'mDisplayId|DisplayDeviceInfo'
adb shell wm size -d 2
```

On many Samsung devices the built-in screen is display `0` and DeX is display
`2`, but this is not guaranteed. Use the ID shown on the device.

## Run

For a DeX display whose ID is `2`:

```sh
export QT_QPA_PLATFORM=minimal
export DESKFLOW_INPUT_BACKEND=adb
export DESKFLOW_ANDROID_DISPLAY_ID=2
export DESKFLOW_ANDROID_MOUSE_MODE=uhid
export DESKFLOW_ANDROID_KEYBOARD_MODE=uhid
termux-wake-lock
build-termux-adb/bin/deskflow-core client --settings "$PWD/termux-client.conf"
```

On a Termux-native Android build the ADB backend is also the default when
`DESKFLOW_INPUT_BACKEND` is unset. Keep the explicit value while testing so the
selected path is obvious.

Optional environment variables:

| Variable | Meaning |
|---|---|
| `DESKFLOW_ANDROID_DISPLAY_ID` | Input target display; defaults to `0` |
| `DESKFLOW_ANDROID_MOUSE_MODE` | `uhid` (default) or SDK pointer injection fallback |
| `DESKFLOW_ANDROID_MOUSE_SCALE` | Optional UHID relative-motion multiplier; defaults to `1.0` |
| `DESKFLOW_ANDROID_KEYBOARD_MODE` | `uhid` (default) or SDK key injection fallback |
| `DESKFLOW_ADB_SERIAL` | Select an ADB device when more than one is connected |
| `DESKFLOW_ADB` | Override the `adb` executable path |
| `DESKFLOW_ADB_BRIDGE` | Override the local generated DEX path |
| `DESKFLOW_ANDROID_SIZE` | Fallback display size such as `1920x1080` if `wm size` fails |

Use `termux-wake-unlock` after stopping the client if the wake lock is no
longer needed.

## Persistent service and boot startup

For normal use, install the client as a `termux-services` runit service. This
keeps it independent of the shell, SSH, or Codex session that started it:

```sh
pkg install termux-api termux-services
./scripts/install-termux-dex-service.sh \
  --server 192.168.1.10 \
  --name dex-phone \
  --display 2 \
  --adb-serial localhost:CONNECTION_PORT
```

If a working settings file already contains the trusted server certificate
state, preserve it with `--settings /absolute/path/to/termux-client.conf`
instead of generating a new one. The installer saves device-specific values in
`~/.config/deskflow-termux-dex/service.env`; edit that file and restart the
service when an address or display ID changes.

Useful commands:

```sh
sv status "$PREFIX/var/service/deskflow-dex"
sv restart "$PREFIX/var/service/deskflow-dex"
sv down "$PREFIX/var/service/deskflow-dex"
tail -f "$HOME/.local/state/deskflow-termux-dex/log/current"
```

For startup immediately after Android boots, install **Termux:API** and
**Termux:Boot** from the same source as Termux and open them once so Android
permits their integrations. The installer creates
`~/.termux/boot/start-services`, which acquires a wake lock and starts the runit
service directory. Also disable battery optimization for Termux and both
add-ons.

Android may turn Wireless debugging off during reboot, and its TLS connection
port is not guaranteed to remain stable. Pairing authorization normally
persists, but if automatic discovery is unavailable in the device's
`android-tools` build, enable Wireless debugging again and run:

```sh
adb connect localhost:NEW_CONNECTION_PORT
sv restart "$PREFIX/var/service/deskflow-dex"
```

The supervisor accepts a sole manually connected ADB device even if the serial
saved by the installer used the previous port. Fully automatic recovery while
Wireless debugging itself is disabled is intentionally impossible without an
APK, root access, or another privileged component.

## Troubleshooting

`no authorized adb device is connected`

: Re-run `adb connect localhost:CONNECTION_PORT` and confirm `adb devices -l`
  says `device`, not `offline` or `unauthorized`.

The cursor moves on the phone instead of DeX

: Re-check `DESKFLOW_ANDROID_DISPLAY_ID` while DeX is active. Samsung may expose
  a separate input-facing display on some One UI versions; inspect all display
  IDs and test them with `adb shell input -d ID tap X Y`.

The cursor stops before reaching the right or bottom edge

: Deskflow tracks an absolute client position while a UHID mouse retains its
  previous relative Android position. On Samsung DeX, the client reads the
  SurfaceFlinger cursor layer when entering the screen and synchronizes the two
  positions. Other vendors may not expose this diagnostic cursor row. The
  optional `DESKFLOW_ANDROID_MOUSE_SCALE` remains available for device-specific
  tuning, but a fixed multiplier cannot fully cancel velocity-dependent Android
  pointer acceleration. On the Deskflow server, enable **Use relative mouse
  movements** under **Configure Server > Advanced**. The server will then
  continue forwarding motion beyond its logical edge without requiring Scroll
  Lock. The Android backend also advertises this requirement in its screen-info
  capabilities, so it works for any client name and for multiple Termux/DeX
  clients even when an external server configuration overrides the GUI setting.
  Before switching away from a relative-only client, the server asks for the
  visible DeX cursor position and leaves only after the physical edge is
  confirmed. This prevents Android pointer acceleration from causing an early
  return to the server while the visible cursor is still far from the edge.

The client is killed in the background

: Disable battery optimization for Termux, use `termux-wake-lock`, and on
  affected Android releases review the developer option for disabling child
  process restrictions.

Korean characters do not compose

: The backend sends physical keyboard scan codes. Select a Korean-capable IME
  on Android/DeX and configure its external-keyboard language shortcut. The
  Hangul key, `Ctrl+Alt+K`, and `Shift+Space` are transported as physical key
  combinations; which one changes language remains an IME setting. Unicode
  text is supported through clipboard sharing, but not as direct key-event
  injection.

Modifier or function-key shortcuts do not work

: Confirm startup logs contain `keyboard=uhid`. Check Android's device view with
  `adb shell dumpsys input`; `Deskflow virtual keyboard` should show
  `Classes: KEYBOARD | ALPHAKEY | EXTERNAL` and `IsExternal: true`. If UHID is
  blocked by a vendor kernel, the client falls back to SDK key events and logs
  a warning.

Input stops after wireless debugging is disabled

: This is expected. The bridge deliberately depends on the revocable ADB shell
  authorization rather than installing an Accessibility or privileged app.

## Design references

- [AOSP input-injection security change](https://android.googlesource.com/platform/frameworks/base/+/edff3851325467a3f56ebe87af67df326b00a318)
  documents why global injection requires `INJECT_EVENTS` and remains available
  to the Android `shell` user.
- [AOSP `input` command source](https://android.googlesource.com/platform/frameworks/base/+/1cff983/cmds/input/src/com/android/commands/input/Input.java)
  shows display-targeted key and pointer injection.
- [scrcpy development documentation](https://github.com/Genymobile/scrcpy/blob/master/doc/develop.md)
  provides a production precedent for an ADB-launched server using the hidden
  `InputManager.injectInputEvent()` API.
- [Samsung DeX architecture](https://developer.samsung.com/samsung-dex/how-it-works.html)
  describes DeX as Android multi-window desktop mode with mouse and keyboard
  input.
