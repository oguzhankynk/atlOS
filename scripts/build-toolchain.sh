#!/bin/sh
# Build the x86_64-elf cross toolchain atlOS needs: binutils and GCC (C, C++)
# with libgcc built for the kernel code model. See docs/building.md.
#
# Usage: scripts/build-toolchain.sh [prefix]     (default: $HOME/opt/cross)
#
# Environment:
#   JOBS        parallel jobs (default: nproc)
#   GNU_MIRROR  where to download from (default: https://ftpmirror.gnu.org)
#   SRC_DIR     directory with already downloaded tarballs (they are verified too)
#
# Host packages (Debian/Ubuntu):
#   build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo curl
#
# Afterwards add <prefix>/bin to PATH.

set -e

PREFIX=${1:-$HOME/opt/cross}
TARGET=x86_64-elf
JOBS=${JOBS:-$(nproc)}
GNU_MIRROR=${GNU_MIRROR:-https://ftpmirror.gnu.org}

BINUTILS_VERSION=2.41
BINUTILS_SHA256=ae9a5789e23459e59606e6714723f2d3ffc31c03174191ef0d015bdf06007450
GCC_VERSION=13.2.0
GCC_SHA256=e275e76442a6067341a27f04c5c6b83d8613144004c0413528863dc6b5c743da

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# fetch <file> <url> <sha256>: copy from SRC_DIR or download, then verify
fetch() {
    if [ -n "$SRC_DIR" ] && [ -f "$SRC_DIR/$1" ]; then
        cp "$SRC_DIR/$1" "$WORK/$1"
    else
        curl -fsSL --retry 3 -o "$WORK/$1" "$2"
    fi
    echo "$3  $WORK/$1" | sha256sum -c --quiet -
}

echo "Building $TARGET toolchain into $PREFIX (binutils $BINUTILS_VERSION, GCC $GCC_VERSION, $JOBS jobs)"

fetch binutils-$BINUTILS_VERSION.tar.xz \
    "$GNU_MIRROR/binutils/binutils-$BINUTILS_VERSION.tar.xz" $BINUTILS_SHA256
fetch gcc-$GCC_VERSION.tar.xz \
    "$GNU_MIRROR/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz" $GCC_SHA256

cd "$WORK"
tar -xf binutils-$BINUTILS_VERSION.tar.xz
tar -xf gcc-$GCC_VERSION.tar.xz

mkdir -p "$PREFIX"
export PATH="$PREFIX/bin:$PATH"

mkdir build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror
make -j"$JOBS"
make install
cd ..

# No headers: the kernel brings its own libc. libgcc is built for the kernel's
# code model (large, no red zone).
mkdir build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc -j"$JOBS"
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large -mno-red-zone' -j"$JOBS"
make install-gcc
make install-target-libgcc

"$PREFIX/bin/$TARGET-gcc" --version | head -1
echo "Done. Add $PREFIX/bin to PATH."
