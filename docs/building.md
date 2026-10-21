# Building and running

## Requirements

| Tool | Tested version | Used for |
|---|---|---|
| `x86_64-elf-gcc`, `x86_64-elf-g++`, binutils | GCC 13.2.0, binutils 2.41 | cross compiling (see below) |
| CMake | 3.22 or newer | build |
| Ninja | any | build (generator of the presets) |
| `grub-mkrescue` + `xorriso` | GRUB 2 | bootable ISO |
| `qemu-system-x86_64` | 6.2 | run, debug, self tests |
| `sfdisk`, `mkfs.fat`, mtools | util-linux 2.37, dosfstools 4.2, mtools 4.0 | the test disk (`disk-image`), no root needed; optional: without them `run` and `debug` boot without a disk, the disk self tests are skipped and `shelltest` is not available |
| Doxygen | optional | API docs |
| clang-format, clang-tidy | 14 (as in CI), optional | `format`, `format-check`, `tidy`; other versions may format differently |

On Debian/Ubuntu:

```sh
sudo apt install cmake ninja-build grub-pc-bin grub-common xorriso mtools qemu-system-x86 dosfstools fdisk doxygen
```

## Cross compiler

atlOS has to be built with a cross compiler that targets `x86_64-elf`; the
host compiler will not work. `scripts/build-toolchain.sh` builds one
(binutils 2.41, GCC 13.2.0 for C and C++, libgcc for the kernel code model);
the downloaded sources are checked against pinned SHA-256 sums. CI builds its
toolchain with the same script.

```sh
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo curl
scripts/build-toolchain.sh              # into $HOME/opt/cross (takes 15-30 minutes)
export PATH="$HOME/opt/cross/bin:$PATH" # also add this to ~/.bashrc or ~/.zshrc
x86_64-elf-gcc --version
```

A different prefix can be given as the argument; `JOBS` sets the number of
parallel jobs. Background: [OSDev Wiki: GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler).

## Building

`scripts/build.sh` is a shortcut for the CMake presets:

| Command | What it does |
|---|---|
| `scripts/build.sh` / `scripts/build.sh all` | build libc, kernel and `build/default/atlOS.iso` |
| `scripts/build.sh iso` | build the ISO |
| `scripts/build.sh run` | boot the ISO in QEMU with the test disk, the serial console (boot messages, then the shell) on the terminal (Ctrl-C quits); see the [screenshot](images/ss.png) |
| `scripts/build.sh debug` | boot the ISO in QEMU halted, waiting for gdb on `localhost:1234`; serial on `telnet localhost 4321` |
| `scripts/build.sh selftest` | build the self test kernel in `build/selftest/` and run it, see [testing.md](testing.md) |
| `scripts/build.sh crashtest` | build a kernel that faults on purpose in `build/crashtest/` and check its report |
| `scripts/build.sh shelltest` | boot the normal kernel and drive its shell over the serial port, see [testing.md](testing.md) |
| `scripts/build.sh docs` | Doxygen API docs into `build/docs/html` |
| `scripts/build.sh format` / `format-check` | clang-format the sources / check them, see [style.md](style.md) |
| `scripts/build.sh tidy` | clang-tidy on the kernel sources |
| `scripts/build.sh clean` | remove `build/` (all presets) |

Commands can be chained: `scripts/build.sh clean all run`.

Builds are incremental. Each preset builds in `build/<preset>/`;
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
| `default` | `build/default/` | normal kernel: mounts the disks and starts the shell |
| `selftest` | `build/selftest/` | `ATLOS_SELFTEST=ON`: the kernel runs the self tests and halts with the result |
| `crashtest` | `build/crashtest/` | `ATLOS_CRASHTEST=ON`: the kernel page faults on purpose after boot |

`selftest` and `crashtest` also set `ATLOS_DEBUG_EXIT_PORT=0xf4`: the exit
status is written to that port when the kernel halts, see
[testing.md](testing.md). Leave it empty (`-DATLOS_DEBUG_EXIT_PORT=`) for a
test kernel meant for real hardware.

All presets export `compile_commands.json` for editors and clangd.

### Outputs

| File | Description |
|---|---|
| `build/default/kernel/atlOS.kernel` | the kernel ELF (with debug info) |
| `build/default/libc/libk.a` | the C library linked into the kernel |
| `build/default/atlOS.iso` | bootable GRUB ISO |
| `build/default/disk.img` | the test disk (`--target disk-image`; `run` and `debug` attach it as the primary ATA master) |

## Debugging

```sh
scripts/build.sh debug          # QEMU waits for gdb
gdb build/default/kernel/atlOS.kernel -ex 'target remote localhost:1234'
```

In VS Code, start the "Run QEMU Debug" task, then the "Debug Kernel" launch
configuration (`.vscode/launch.json`).

## FAT test disk

`tools/disk/create_disk_image.sh [12|16]` creates `tools/disk/disk.img`, a
128 MB image with four FAT partitions, and `tools/disk/copy.sh` copies
`MY_HELLO.txt` onto the first one. Both use `sudo` and `/dev/loop0`. The disk
drivers are not wired up yet, so the image is not attached by the run targets.
Details, requirements and how to attach it to QEMU by hand:
[`tools/disk/README.md`](../tools/disk/README.md).

## User programs

The programs in `user/` (ring 3, see [user/README.md](../user/README.md)) are
built with every preset into `build/<preset>/user/*.elf` and copied to the
test disk as `/BIN/<NAME>.ELF`. Run one from the shell:
`run /disk0/BIN/HELLO.ELF`.

## Screenshots

`scripts/take-screenshots.py <iso> <disk.img> <dir>` boots the normal kernel,
types a few shell commands over the serial port and saves the VGA screen
(QEMU's `screendump`) as `boot.png` and `shell.png`, e.g. for the wiki.

