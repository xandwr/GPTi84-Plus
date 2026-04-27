#!/usr/bin/env bash
# Incremental build for the Pico firmware.
# Wipe build/ manually for a true clean rebuild; ccache survives that.
#
# Usage:
#   ./build.sh           configure (if needed) and build
#   ./build.sh --flash   build, then flash via picotool (Pico must be in BOOTSEL)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/firmware/c"
BUILD="$ROOT/build"

FLASH=0
for arg in "$@"; do
    case "$arg" in
        --flash) FLASH=1 ;;
        *) echo "unknown arg: $arg" >&2; exit 2 ;;
    esac
done

if ! command -v ccache >/dev/null; then
    echo "ccache not found on PATH" >&2
    exit 1
fi

if [ ! -f "$BUILD/build.ninja" ] && [ ! -f "$BUILD/Makefile" ]; then
    cmake -S "$SRC" -B "$BUILD" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
fi

cmake --build "$BUILD"

ln -sf "$BUILD/compile_commands.json" "$ROOT/compile_commands.json"

if [ "$FLASH" = "1" ]; then
    picotool load -x "$BUILD/bringup.uf2"
fi
