#!/bin/sh
# Shortcut for the CMake presets, see docs/building.md.
#
# Usage: scripts/build.sh [command...]
#   all       build libc, kernel and the ISO (default)
#   iso       build the ISO
#   run       boot the ISO in QEMU, serial on the terminal (Ctrl-A X quits)
#   debug     boot the ISO in QEMU and wait for gdb on :1234
#   selftest  build the self test kernel and run it (exit code 0 = passed)
#   docs      generate the Doxygen API docs into build/docs
#   clean     remove build/ and build-selftest/

set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

usage() {
    sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
}

# build <preset> [cmake --build args...]
build() {
    preset=$1
    shift

    case $preset in
        default) dir=build ;;
        selftest) dir=build-selftest ;;
    esac

    if [ ! -f "$dir/CMakeCache.txt" ]; then
        cmake --preset "$preset"
    fi
    cmake --build --preset "$preset" "$@"
}

if [ $# -eq 0 ]; then
    set -- all
fi

for cmd in "$@"; do
    case $cmd in
        all) build default ;;
        iso | run | debug | docs) build default --target "$cmd" ;;
        selftest) build selftest --target selftest ;;
        clean) rm -rf build build-selftest ;;
        -h | --help | help) usage ;;
        *)
            echo "unknown command: $cmd" >&2
            usage >&2
            exit 2
            ;;
    esac
done
