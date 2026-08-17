<!--
SPDX-FileCopyrightText: 2026 Andy D.K Lee
SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
-->

# Deskflow for macOS and Logi Options+

This is a macOS-focused fork of [Deskflow](https://github.com/deskflow/deskflow) for testing and using input improvements that are not available in the upstream release.

The main use case is a Mac acting as the Deskflow server with a Logitech mouse configured through Logi Options+, while Linux or Windows computers are used as clients.

## Included improvements

- Opt-in forwarding of macOS Back and Forward navigation actions to remote clients.
- Reliable Caps Lock forwarding from a Mac server, including uppercase and lowercase state on Linux and Windows clients.
- An emergency shortcut to return control to the Mac if a client disconnects: `Control + Option + Command + Escape`.
- An optional ASCII input-source mode while controlling another computer, with restoration of the previous macOS input source on return.

## Navigation forwarding

Logi Options+ can turn mouse Back and Forward actions into macOS navigation events. They work locally on the Mac but are not normally delivered to a Deskflow client.

To forward them:

1. Open **Configure Server**.
2. Select **Advanced**.
3. Enable **Forward macOS navigation gestures**.

The option is disabled by default. It assumes the usual Back and Forward mapping and may not suit custom button mappings.

## Status

The navigation feature has been manually tested with a macOS server and both Linux and Windows clients. With Logi Options+ stopped, the mouse uses Deskflow's existing extra-button handling and does not need this option.

This fork is intended to remain close to upstream Deskflow. Features may be proposed upstream after broader testing and, where necessary, a more general design.

## Experimental Termux and Samsung DeX client

The `android-termux-dex` branch adds a headless Termux client that controls the
Android or Samsung DeX display through a local wireless-ADB input bridge. It
does not install an Android APK.

### Install on an Android device

Install Termux, Termux:API, and Termux:Boot from the same trusted source. Open
the two add-ons once, grant their requested permissions, and disable battery
optimization for all three applications. In Termux:

```sh
pkg update
pkg install x11-repo
pkg install git cmake clang make pkg-config openssl android-tools openjdk-17 \
  d8 libxkbcommon qt6-qtbase qt6-qtbase-gtk-platformtheme qt6-qttools \
  termux-api termux-services

git clone --branch android-termux-dex \
  https://github.com/andy5090/deskflow-macos-logi-options-plus.git
cd deskflow-macos-logi-options-plus
./scripts/build-termux-adb.sh
```

Enable Android **Wireless debugging**, pair once, and connect using the separate
connection port shown by Android:

```sh
adb pair localhost:PAIRING_PORT
adb connect localhost:CONNECTION_PORT
adb devices -l
```

Install and start the persistent service. Use a unique `--name` for every
Android client, add that same name to the Deskflow server layout, and use the
DeX display ID reported by that device:

```sh
./scripts/install-termux-dex-service.sh \
  --server 192.168.1.10 \
  --name dex-phone \
  --display 2 \
  --adb-serial localhost:CONNECTION_PORT
```

The installer creates a runit service independent of the current terminal or
Codex session and adds the Termux:Boot hook needed after a reboot. Check it with
`sv status $PREFIX/var/service/deskflow-dex`; logs are stored under
`~/.local/state/deskflow-termux-dex/log/`. Wireless debugging may be disabled
and its connection port may change after an Android reboot. In that case,
enable it again, run `adb connect localhost:NEW_PORT`, then restart the service
with `sv restart $PREFIX/var/service/deskflow-dex`.

See the [Termux and DeX client guide](docs/termux-dex.md) for the architecture,
security tradeoffs, manual operation, service configuration, and limitations.

## Branches

- `master` tracks upstream Deskflow without fork-specific changes.
- `macos-logi-options-plus` is the default branch for this fork and contains the macOS improvements.
- `android-termux-dex` experiments with an APK-free Termux/DeX client.
- Future upstream contributions will use clean `contrib/*` branches created from the latest upstream branch.

## Building

See [the Deskflow build guide](docs/dev/build.md). Local macOS development builds can be signed with an Apple Development certificate using the `APPLE_CODESIGN_DEV` CMake option described there.

## Upstream project

Deskflow is a free and open source keyboard and mouse sharing application. For upstream releases, documentation, support, and general development, visit:

- [Deskflow repository](https://github.com/deskflow/deskflow)
- [Deskflow website](https://deskflow.org)

This is an unofficial community fork and is not affiliated with Logitech. Logi Options+ and Logitech are trademarks of Logitech Europe S.A. and/or its affiliates.

## License

This project follows Deskflow's [GPL-2.0 license with the OpenSSL exception](LICENSE).
