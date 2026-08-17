#!/data/data/com.termux/files/usr/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
service_name=deskflow-dex
config_dir=${XDG_CONFIG_HOME:-"$HOME/.config"}/deskflow-termux-dex
state_dir=${XDG_STATE_HOME:-"$HOME/.local/state"}/deskflow-termux-dex
service_dir=$PREFIX/var/service/$service_name
boot_dir=$HOME/.termux/boot
boot_script=$boot_dir/start-services
server_address=
client_name=
display_id=2
adb_serial=
settings_path=
build_dir=
start_service=true

show_help()
{
  printf '%s\n' \
    "Usage: $0 [options]" \
    "" \
    "Install a persistent Termux/runit Deskflow DeX client." \
    "" \
    "Options:" \
    "  --server HOST       Deskflow server address (required when creating settings)" \
    "  --name NAME         Deskflow client screen name (default: dex-DEVICE)" \
    "  --display ID        Android/DeX display ID (default: 2)" \
    "  --adb-serial SERIAL Wireless ADB connection address; defaults to the sole connected device" \
    "  --settings PATH     Reuse an existing Deskflow client settings file" \
    "  --build-dir PATH    Build directory containing bin/deskflow-core" \
    "  --no-start          Install the service disabled without starting it" \
    "  -h, --help          Show this help"
}

while [ "$#" -gt 0 ]; do
  case $1 in
  --server)
    server_address=${2:?missing value for --server}
    shift 2
    ;;
  --name)
    client_name=${2:?missing value for --name}
    shift 2
    ;;
  --display)
    display_id=${2:?missing value for --display}
    shift 2
    ;;
  --adb-serial)
    adb_serial=${2:?missing value for --adb-serial}
    shift 2
    ;;
  --settings)
    settings_path=${2:?missing value for --settings}
    shift 2
    ;;
  --build-dir)
    build_dir=${2:?missing value for --build-dir}
    shift 2
    ;;
  --no-start)
    start_service=false
    shift
    ;;
  -h | --help)
    show_help
    exit 0
    ;;
  *)
    printf 'Unknown option: %s\n' "$1" >&2
    show_help >&2
    exit 2
    ;;
  esac
done

case $display_id in
'' | *[!0-9]*)
  printf 'Display ID must be a non-negative integer: %s\n' "$display_id" >&2
  exit 2
  ;;
esac

if [ -z "$build_dir" ]; then
  for candidate_dir in "$project_root/build-termux-adb" "$project_root/build-termux-make"; do
    if [ -x "$candidate_dir/bin/deskflow-core" ]; then
      build_dir=$candidate_dir
      break
    fi
  done
fi
if [ -z "$build_dir" ]; then
  build_dir=$project_root/build-termux-adb
fi
case $build_dir in
/*) ;;
*) build_dir=$project_root/$build_dir ;;
esac
deskflow_binary=$build_dir/bin/deskflow-core

if [ ! -x "$deskflow_binary" ]; then
  printf 'Deskflow has not been built at %s\nRun ./scripts/build-termux-adb.sh first.\n' "$deskflow_binary" >&2
  exit 1
fi
for required_command in adb sv svlogd termux-wake-lock; do
  if ! command -v "$required_command" >/dev/null 2>&1; then
    printf 'Missing command: %s\nInstall android-tools, termux-api, and termux-services.\n' "$required_command" >&2
    exit 1
  fi
done

if [ -z "$adb_serial" ]; then
  detected_devices=$(adb devices 2>/dev/null | awk 'NR > 1 && $2 == "device" { print $1 }')
  detected_count=$(printf '%s\n' "$detected_devices" | awk 'NF { count++ } END { print count + 0 }')
  if [ "$detected_count" -eq 1 ]; then
    adb_serial=$detected_devices
  fi
fi

if [ -z "$client_name" ]; then
  device_name=$(getprop ro.product.device 2>/dev/null || true)
  if [ -z "$device_name" ]; then
    device_name=termux
  fi
  client_name=dex-$device_name
fi

mkdir -p "$config_dir" "$state_dir/log" "$service_dir/log" "$boot_dir"

if [ -n "$settings_path" ]; then
  case $settings_path in
  /*) ;;
  *) settings_path=$(CDPATH= cd -- "$(dirname -- "$settings_path")" && pwd)/$(basename -- "$settings_path") ;;
  esac
  if [ ! -r "$settings_path" ]; then
    printf 'Settings file is not readable: %s\n' "$settings_path" >&2
    exit 1
  fi
else
  if [ -z "$server_address" ]; then
    if [ -t 0 ]; then
      printf 'Deskflow server address: '
      IFS= read -r server_address
    fi
  fi
  if [ -z "$server_address" ]; then
    printf 'Use --server HOST when no existing --settings file is supplied.\n' >&2
    exit 2
  fi
  settings_path=$config_dir/client.conf
  umask 077
  {
    printf '%s\n' '[client]'
    printf 'languageSync=false\nremoteHost=%s\n\n' "$server_address"
    printf '%s\n' '[core]'
    printf 'computerName=%s\nport=24800\n\n' "$client_name"
    printf '%s\n' '[log]' 'level=INFO' '' '[security]' 'tlsEnabled=true'
  } >"$settings_path"
fi

shell_quote()
{
  printf "'"
  printf '%s' "$1" | sed "s/'/'\\\\''/g"
  printf "'"
}

env_path=$config_dir/service.env
umask 077
{
  printf 'DESKFLOW_BINARY=%s\n' "$(shell_quote "$deskflow_binary")"
  printf 'DESKFLOW_SETTINGS=%s\n' "$(shell_quote "$settings_path")"
  printf 'DESKFLOW_ADB_SERIAL=%s\n' "$(shell_quote "$adb_serial")"
  printf 'DESKFLOW_ANDROID_DISPLAY_ID=%s\n' "$(shell_quote "$display_id")"
  printf "DESKFLOW_ANDROID_MOUSE_MODE='uhid'\n"
  printf "DESKFLOW_ANDROID_KEYBOARD_MODE='uhid'\n"
  printf "QT_QPA_PLATFORM='minimal'\n"
  printf "DESKFLOW_INPUT_BACKEND='adb'\n"
} >"$env_path"

runner_path=$script_dir/run-termux-dex-client.sh
service_run=$service_dir/run
service_log_run=$service_dir/log/run
{
  printf '%s\n' '#!/data/data/com.termux/files/usr/bin/sh'
  printf 'export DESKFLOW_TERMUX_ENV=%s\n' "$(shell_quote "$env_path")"
  printf 'exec %s 2>&1\n' "$(shell_quote "$runner_path")"
} >"$service_run"
{
  printf '%s\n' '#!/data/data/com.termux/files/usr/bin/sh'
  printf 'exec svlogd -tt %s\n' "$(shell_quote "$state_dir/log")"
} >"$service_log_run"
chmod 700 "$service_run" "$service_log_run" "$runner_path"

if [ ! -e "$boot_script" ]; then
  {
    printf '%s\n' '#!/data/data/com.termux/files/usr/bin/sh'
    printf '%s\n' 'termux-wake-lock'
    printf '%s\n' '. /data/data/com.termux/files/usr/etc/profile.d/start-services.sh'
  } >"$boot_script"
  chmod 700 "$boot_script"
elif ! grep -q 'profile.d/start-services.sh' "$boot_script"; then
  printf '%s\n' '. /data/data/com.termux/files/usr/etc/profile.d/start-services.sh' >>"$boot_script"
fi

if [ "$start_service" = true ]; then
  SVDIR=$PREFIX/var/service sv-enable "$service_name" >/dev/null
else
  touch "$service_dir/down"
  sv down "$service_dir" >/dev/null 2>&1 || true
fi

printf '%s\n' \
  "Installed Deskflow DeX service: $service_dir" \
  "Configuration: $env_path" \
  "Settings: $settings_path" \
  "Logs: $state_dir/log/current" \
  "Status: sv status $service_dir" \
  "Restart: sv restart $service_dir"
if [ -z "$adb_serial" ]; then
  printf '%s\n' 'No ADB serial was saved. Connect exactly one authorized device before the service starts.'
fi
printf '%s\n' 'For boot startup, install Termux:Boot from the same source as Termux and open it once.'
