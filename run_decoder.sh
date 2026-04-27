#!/usr/bin/env bash
# Usage:
#   ./run_decoder.sh [args...]   Build (if needed) and run the host-side .8Xu decoder.
#                                Extra args are forwarded to the decoder binary.
#   ./run_decoder.sh -c          Wipe the decoder build directory and exit.
#   ./run_decoder.sh -h          Show this help.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src_dir="$repo_root/ti84_plus"
build_dir="$src_dir/build"
binary="$build_dir/decoder"
default_input="$src_dir/ti84_plus_255/TI84Plus_OS255.8Xu"

usage() { sed -n '2,6p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

case "${1:-}" in
    -h|--help)  usage; exit 0 ;;
    -c|--clean) rm -rf "$build_dir"; echo "Removed $build_dir"; exit 0 ;;
esac

if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
    cmake -S "$src_dir" -B "$build_dir"
fi
cmake --build "$build_dir" --target decoder -j

if [[ $# -gt 0 ]]; then
    exec "$binary" "$@"
else
    exec "$binary" "$default_input"
fi
