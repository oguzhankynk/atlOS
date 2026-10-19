# Building and running

## Requirements

| Tool | Tested version | Used for |
|---|---|---|
| `x86_64-elf-gcc`, `x86_64-elf-g++`, binutils | GCC 13.2.0, binutils 2.41 | cross compiling (see below) |
| CMake | 3.22 or newer | build |
| Ninja | any | build (generator of the presets) |
| `grub-mkrescue` + `xorriso` | GRUB 2 | bootable ISO |
| `qemu-system-x86_64` | 6.2 | run, debug, self tests |
| Doxygen | optional | API docs |

On Debian/Ubuntu:

```sh
sudo apt install cmake ninja-build grub-pc-bin grub-common xorriso mtools qemu-system-x86 doxygen
```

## Cross compiler

atlOS has to be built with a cross compiler that targets `x86_64-elf`; the
host compiler will not work. The steps below build one in `$HOME/opt/cross`.
More background: [OSDev Wiki: GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler).

```sh
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"     # also add this to ~/.bashrc or ~/.zshrc

sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget
mkdir -p $HOME/src && cd $HOME/src
```

Binutils:

```sh
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
tar -xf binutils-2.41.tar.xz
mkdir build-binutils && cd build-binutils
../binutils-2.41/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..
```

GCC (C and C++, no headers, libgcc built for the kernel code model):

```sh
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz
tar -xf gcc-13.2.0.tar.xz
mkdir build-gcc && cd build-gcc
../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large -mno-red-zone' -j$(nproc)
make install-gcc
make install-target-libgcc
```

Check:

```sh
x86_64-elf-gcc --version
x86_64-elf-g++ --version
```

## Building

`scripts/build.sh` is a shortcut for the CMake presets:

| Command | What it does |
|---|---|
| `scripts/build.sh` / `scripts/build.sh all` | build libc, kernel and `build/atlOS.iso` |
| `scripts/build.sh iso` | build the ISO |
| `scripts/build.sh run` | boot the ISO in QEMU, serial output on the terminal (Ctrl-C quits) |
| `scripts/build.sh debug` | boot the ISO in QEMU halted, waiting for gdb on `localhost:1234`; serial on `telnet localhost 4321` |
| `scripts/build.sh selftest` | build the self test kernel in `build-selftest/` and run it, see [testing.md](testing.md) |
| `scripts/build.sh docs` | Doxygen API docs into `build/docs/html` |
| `scripts/build.sh clean` | remove `build/` and `build-selftest/` |

Commands can be chained: `scripts/build.sh clean all run`.

Builds are incremental. Everything goes into `build/` (or `build-selftest/`);
nothing is written into the source tree.

### With CMake directly

```sh
cmake --preset default                        # configure build/
cmake --build --preset default                # kernel + ISO
cmake --build --preset default --target run   # or iso, debug, docs

cmake --preset selftest
cmake --build --preset selftest --target selftest
ctest --preset selftest                       # the same test through CTest
```

| Preset | Build directory | Notes |
|---|---|---|
| `default` | `build/` | normal kernel with the demo threads |
| `selftest` | `build-selftest/` | `ATLOS_SELFTEST=ON`: the kernel runs the self tests and exits QEMU |

Both presets export `compile_commands.json` for editors and clangd.

### Outputs

| File | Description |
|---|---|
| `build/kernel/atlOS.kernel` | the kernel ELF (with debug info) |
| `build/libc/libk.a` | the C library linked into the kernel |
| `build/atlOS.iso` | bootable GRUB ISO |

## Debugging

```sh
scripts/build.sh debug          # QEMU waits for gdb
gdb build/kernel/atlOS.kernel -ex 'target remote localhost:1234'
```

In VS Code, start the "Run QEMU Debug" task, then the "Debug Kernel" launch
configuration (`.vscode/launch.json`).

## FAT test disk

`tools/disk/create_disk_image.sh [12|16]` creates `tools/disk/disk.img`, a
128 MB image with four FAT partitions, and `tools/disk/copy.sh` copies
`MY_HELLO.txt` onto the first one. Both use `sudo` and `/dev/loop0`. The disk
drivers are not wired up yet, so the image is not attached by the run targets.
