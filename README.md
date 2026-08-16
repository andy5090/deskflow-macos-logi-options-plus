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

## Branches

- `master` tracks upstream Deskflow without fork-specific changes.
- `macos-logi-options-plus` is the default branch for this fork and contains the macOS improvements.
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
