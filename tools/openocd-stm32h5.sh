#!/usr/bin/env bash
set -euo pipefail

CUBEIDE_APP="${CUBEIDE_APP:-/Applications/STM32CubeIDE.app}"

if [[ ! -d "$CUBEIDE_APP" ]]; then
    echo "ERROR: STM32CubeIDE.app not found at: $CUBEIDE_APP" >&2
    exit 1
fi

OPENOCD_BIN="$(find "$CUBEIDE_APP" \
    -path "*externaltools.openocd.macos64*/tools/bin/openocd" \
    -type f \
    | sort -V \
    | tail -n 1)"

ST_SCRIPTS_DIR="$(find "$CUBEIDE_APP" \
    -path "*debug.openocd*/resources/openocd/st_scripts" \
    -type d \
    | sort -V \
    | tail -n 1)"

INTERFACE_CFG="$ST_SCRIPTS_DIR/interface/stlink-dap.cfg"
TARGET_CFG="$ST_SCRIPTS_DIR/target/stm32h5x.cfg"

if [[ -z "$OPENOCD_BIN" || ! -x "$OPENOCD_BIN" ]]; then
    echo "ERROR: OpenOCD binary not found in STM32CubeIDE.app" >&2
    exit 1
fi

if [[ -z "$ST_SCRIPTS_DIR" || ! -d "$ST_SCRIPTS_DIR" ]]; then
    echo "ERROR: ST OpenOCD script directory not found" >&2
    exit 1
fi

if [[ ! -f "$INTERFACE_CFG" ]]; then
    echo "ERROR: Missing interface config: $INTERFACE_CFG" >&2
    exit 1
fi

if [[ ! -f "$TARGET_CFG" ]]; then
    echo "ERROR: Missing target config: $TARGET_CFG" >&2
    exit 1
fi

echo "Using OpenOCD:"
echo "  $OPENOCD_BIN"
echo "Using scripts:"
echo "  $ST_SCRIPTS_DIR"
echo "Using interface:"
echo "  $INTERFACE_CFG"
echo "Using target:"
echo "  $TARGET_CFG"
echo

exec "$OPENOCD_BIN" \
    -s "$ST_SCRIPTS_DIR" \
    -c "gdb_report_data_abort enable" \
    -c "gdb_port 3333" \
    -c "tcl_port 6666" \
    -c "telnet_port 4444" \
    -f "$INTERFACE_CFG" \
    -f "$TARGET_CFG"