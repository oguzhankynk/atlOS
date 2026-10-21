# Changelog

All notable changes to atlOS are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow
[Semantic Versioning](https://semver.org/); while the version is 0.x, minor
versions may change interfaces. See [docs/releasing.md](docs/releasing.md).

## [Unreleased]

### Added

- Shell commands for the hardware: `cr` (CR0, CR2, CR3, CR4, EFER by bit),
  `rdmsr`/`wrmsr`, `in`/`out` (8, 16, 32 bit), `gdt`, `idt [vector]`,
  `pt addr` (the page table walk), `irq` (interrupts per vector)
- The exception table (`.extable`, `Fault::fixup`): `readMsr`, `writeMsr`
  and `readQword` fault without a panic and return false
- `IDT::interruptCount`, `AddressSpace::walk`, `IOPort::inportl`/`outportl`
- Self tests for the exception table, the walk, the counts and the commands
  (752 checks); the shell test runs them (32 steps)

### Changed

- QEMU emulates the same CPU everywhere: an Intel Skylake
  (`Skylake-Client-v1`) without the features its emulator lacks, in
  `scripts/qemu-cpu`: run, debug, the tests and the screenshots
- `cpuinfo` (`printCPUInfo`): one value per line, the family and model as
  the CPU means them (with the extended fields), only the features the CPU
  has, with their Linux names, wrapped to 80 columns, and the cache sizes
- **No scheduler handed in**: `Threading::currentTask()` (set on every
  switch) and `TCB::scheduler` let `WaitQueue`, `Semaphore(initial, max)`,
  `Mutex`, `ConditionVariable` and `CharacterQueue` be created without a
  scheduler; `Thread(entry)` uses the caller's scheduler and the kernel
  address space; thread entry points are `void()` and use
  `Threading::yield()`, `sleep(ms)`, `exitThread()`; `Process::Runtime` and
  `Shell::Context` no longer take the scheduler

### Fixed

- `strncmp` compared an `int` counter with the `size_t` length (a
  `-Wsign-compare` warning on every build); libc warnings are now errors, as
  in the kernel

- `scripts/build.sh run` failed with "mcopy: not found" on a machine without
  mtools: the test disk tools are now looked up on every configure (no cached
  paths), the disk script uses the ones CMake found and says which are
  missing, and without them `run` and `debug` boot without a disk;
  `scripts/build.sh` configures every time, so installing a tool takes effect
  at once

## [0.3.0] - 2026-10-20

User programs: static ELF64 files from the disk run in ring 3, each in an
address space of its own (physmap, shared kernel half), with `int 0x80`
system calls; a guide to writing them; a new banner.

### Added

- Physmap: all physical memory (up to 64 GB) at `0xFFFF800000000000`, with
  its tables in the kernel image; page tables are edited through it
- User programs (`kernel/proc/`, `user/`): static ELF64 files from the disk
  (`/disk0/BIN`), loaded into an address space of their own and run in ring 3
  on a kernel thread (`iretq`, `TSS.rsp0` set by the scheduler on every
  switch); `int 0x80` system calls `exit` and `write` with user pointer
  checks; a fault in ring 3 ends the program with 128 + vector instead of a
  panic; the shell's `run` command; `HELLO.ELF`, `FAULT.ELF`, `BADPTR.ELF`
- `AddressSpace::isUserAccessible`; `GDT::setKernelStack`; TCB `owner`
- `user/README.md`: a guide to writing user programs (environment, memory
  layout, system calls, exit codes, a first program, debugging a crash, build
  rules); `COUNT.ELF`, its example; user programs are built with `-g`
- `scripts/symbolize.sh` resolves `RIP=` of a register dump too, against any
  ELF file: the kernel or a user program
- Self tests for ELF parsing and the user programs (706 checks); the shell
  test runs programs
- Several address spaces: `AddressSpace(pmm, kernel)` shares the kernel's
  `PML4[0]` and `PML4[256..511]`; `allocateAt`/`freeAt` map zeroed 2 MB user
  pages in `PML4[1..255]`; `activate()`, `isActive()`; the destructor gives
  back pages and tables (owned pages are marked in entry bit 9)

### Changed

- A new banner in plain ASCII, in color; the shell clears the screen and
  shows it with the version, free memory and mounted disks; the boot starts
  with a one line version message
- `VGATextConsole` understands ANSI colors and UTF-8 block characters, so
  the screen and a serial terminal show the same output
- `tasks` and `vmmap` lines fit 80 columns
- `scripts/take-screenshots.py`: the VGA screen after boot and in the shell
- The PMM and VMM no longer log every allocation
- The recursive page table mapping (`PML4[510]`) is gone
- The PMM manages at most the 64 GB the physmap covers

### Fixed

- `InterruptFrame` had `rsi` and `rdi` swapped: register dumps showed them
  the wrong way round, and system call arguments would have been read from
  the wrong registers

## [0.2.0] - 2026-10-20

Fault handling, synchronization, the keyboard, disks with FAT16 and a VFS,
and a kernel shell.

### Added

- CI on GitHub Actions: build, self tests, format and clang-tidy on every push
  and pull request; tagged releases are built and published automatically
- `scripts/build-toolchain.sh` builds the cross toolchain (pinned versions,
  verified downloads)
- TSS with an IST stack for the double fault handler: a broken kernel stack
  is reported instead of resetting the machine (triple fault)
- Exception reports decode the page fault (CR2, read/write/fetch, present,
  user/kernel) and selector error codes (#TS, #NP, #SS, #GP)
- Panic backtrace through the frame pointer chain; `scripts/symbolize.sh`
  turns its addresses into functions and lines
- `crashtest` preset and `scripts/run-crashtest.sh`: a kernel that page faults
  on purpose, its report is checked in CI
- Self tests for fault decoding, the TSS and the backtrace (175 checks)
- `kernel/sync/`: `Atomic<T>`, `InterruptGuard`, `WaitQueue` (FIFO),
  counting `Semaphore` (`take`/`tryTake`/`give`, `give` from interrupt
  handlers), `Mutex` with an owner, `ConditionVariable`; teaching README
- Self tests for the sync objects (265 checks)
- `Drivers::PS2Keyboard` (IRQ 1) behind a new `CharacterSource` interface:
  scancode set 1 decoding (Shift, Caps Lock, Ctrl, extended keys) as a pure
  `ScancodeDecoder`, US layout, a 64 character buffer, `read()` waits on a
  semaphore; self tests (333 checks)
- The VGA console handles backspace
- Storage: `BlockDevice` interface; `ATAPioDevice` (IDENTIFY, 28-bit LBA PIO
  with polling and timeouts), `MemoryBlockDevice`, MBR partition table and
  `PartitionDevice`; read only `Fs::Fat16` (8.3 names, subdirectories,
  checks for broken cluster chains); `Fs::Vfs` with mount points (`/disk0`);
  the ATA disks are probed and their FAT16 partitions mounted at boot
- `scripts/make-disk-image.sh` and the `disk-image` target build the test disk
  without root (sfdisk, mkfs.fat, mtools); the self tests read it in CI
  (439 checks)
- FAT16 writing: `create`, `write` (append and overwrite, clusters allocated
  first fit, every FAT copy updated), `makeDirectory`, `remove`,
  `removeDirectory`, also through `Fs::Vfs`; fixed 1980-01-01 timestamps
- `scripts/check-test-disk.sh`: after the self tests, `fsck.fat` and mtools
  check on the host what the kernel wrote to the test disk (577 checks)
- Kernel shell (`kernel/shell/`), started by the normal kernel: line editor
  (Backspace, Ctrl+C, history on Up/Down), quoted words, a command table:
  `help`, `clear`, `echo` (with `>`/`>>`), `version`, `uptime`, `pwd`, `cd`,
  `ls`, `cat`, `touch`, `mkdir`, `rm`, `rmdir`, `mount`, `mem`, `md`, `mw`,
  `vmmap`, `tasks`/`ps`, `cpuinfo`, `panic`, `fault`, `reboot`
- `Drivers::CharacterQueue`: the keyboard and COM1 (receive interrupt, new)
  feed one queue; the arrow keys arrive as ANSI sequences
- `Drivers::TeeSink`: the console writes to the screen and COM1
- `AddressSpace::translate` (no fault, physical 0 told apart from unmapped,
  4 KB tables followed) and `forEachMapping`; `Memory::heapStatistics`
- TCBs carry a name and their stack range
- `scripts/run-shelltest.py` / `scripts/build.sh shelltest`: the shell driven
  over the serial port in CI; self tests for the parser, line editor and
  commands (651 checks)

### Changed

- clang-tidy findings are errors (`WarningsAsErrors`)
- One panic path: exceptions and `kernelPanic` print a backtrace and halt with
  `cli; hlt`; the exit status goes to an optional debug exit port
  (`ATLOS_DEBUG_EXIT_PORT`, set by the test presets) instead of a hard coded
  test device, so test kernels also run on real hardware
- New threads and the boot flow start with `rbp = 0`
- The scheduler tick counter is a `Sync::Atomic`; idling in `sti; hlt`
  restores the caller's interrupt state
- `Spinlock` uses `Sync::Atomic`
- The default kernel echoes keyboard input instead of running the A/B demo
  threads, and lists the mounted disks first
- The serial port sends `\r\n` for `\n`; the console no longer leaves the
  screen when COM1 exists
- The default kernel starts the shell instead of the keyboard echo demo
- `PS2Keyboard` pushes into a `CharacterQueue` instead of its own buffer
- Removed the old `DeviceManager`, `Device`/`Disk` classes, the untested FAT16
  write code and the `sudo` based disk scripts

### Fixed

- A task that kept the CPU after a sleep (nothing else to run) stayed in the
  Ready state instead of Running

- Exception names for vectors 10..21 were off by one (a page fault was
  reported as "IntelReserved")

## [0.1.0] - 2026-10-20

First release.

### Kernel

- x86_64 higher half kernel booted by GRUB2 (multiboot2): loaded at 4 MB,
  mapped at `0xFFFFFFFF80000000` with 2 MB pages
- GDT, IDT, exception reports with a register dump
- `Drivers::PIC` (8259) and `Drivers::PIT` behind the `InterruptController` and
  `TickSource` interfaces; only the lines a driver uses are unmasked
- `Memory::PhysicalMemoryManager`: 2 MB blocks from the multiboot memory map,
  4 KB frames for page tables, kernel image reserved
- `Memory::AddressSpace`: 2 MB pages through a recursive page table mapping
- kernel heap (`kmalloc`, `kfree`, `krealloc`, `kcalloc`) on liballoc 1.1
- `Threading::Scheduler`: cooperative round robin with `yield`, `sleep`,
  `block` / `unblock` and thread exit; each thread has its own 2 MB stack page
- console on COM1 and the VGA text screen (`Core::setConsole`)
- CPUID helpers, BIOS/SMBIOS parsing, a small freestanding libc
- in the tree but not wired up yet: ATA, FAT16, VFS, keyboard, mutex, semaphore

### Build, tests and tooling

- one CMake project with presets (`build/<preset>`), `scripts/build.sh` for
  `all`, `iso`, `run`, `debug`, `selftest`, `docs`, `format`, `tidy`
- 139 in-kernel self tests run headless in QEMU (`scripts/build.sh selftest`,
  CTest); most build isolated instances on fakes
- WebKit code style with clang-format and clang-tidy; the build fails if SIMD
  instructions end up in the image
- the version comes from `project(atlOS VERSION ...)`; the banner shows it with
  `git describe`

### Fixed during development

- the ISR stubs added the interrupted code's `r12` to `rsp` and overwrote `rax`
- the PMM handed out the kernel's own physical memory
- page directories were linked at the wrong physical address
- the first `yield` overwrote thread 0's context
- thread stacks broke the System V stack alignment
- libc was built with SSE although SSE is never enabled
- serial writes did not wait for the transmitter between bytes

[Unreleased]: https://github.com/oguzhankynk/atlOS/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/oguzhankynk/atlOS/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/oguzhankynk/atlOS/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/oguzhankynk/atlOS/releases/tag/v0.1.0
