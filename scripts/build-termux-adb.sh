#!/data/data/com.termux/files/usr/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

set -eu

build_dir=${1:-build-termux-adb}

cmake -S . -B "$build_dir" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_INSTALLER=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_X11_SUPPORT=OFF \
  -DBUILD_ADB_CLIENT_SUPPORT=ON

cmake --build "$build_dir" --target deskflow-core -j2

printf '%s\n' "Built $build_dir/bin/deskflow-core"
