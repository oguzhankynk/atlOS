#!/bin/sh
# Shortcut for the CMake presets, see docs/building.md.
#
# Usage: scripts/build.sh [command...]
#   all       build libc, kernel and the ISO (default)
#   iso       build the ISO
#   run       boot the ISO in QEMU, serial on the terminal (Ctrl-C quits)
#   debug     boot the ISO in QEMU and wait for gdb on :1234
#   selftest  build the self test kernel and run it (exit code 0 = passed)
#   crashtest build a kernel that page faults on purpose and check its report
#   docs      generate the Doxygen API docs into build/docs
#   format    format kernel and libc sources with clang-format
#   format-check  fail if a source file is not formatted
#   tidy      run clang-tidy on the kernel sources
#   clean     remove build/ (every preset)

set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

usage() {
    sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
}

# build <preset> [cmake --build args...]
build() {
    preset=$1
    shift

    # Every preset builds in build/<preset name> (CMakePresets.json)
    if [ ! -f "build/$preset/CMakeCache.txt" ]; then
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
        iso | run | debug | docs | format | format-check | tidy) build default --target "$cmd" ;;
        selftest) build selftest --target selftest ;;
        crashtest) build crashtest --target crashtest ;;
        clean) rm -rf build ;;
        -h | --help | help) usage ;;
        *)
            echo "unknown command: $cmd" >&2
            usage >&2
            exit 2
            ;;
    esac
done
