# Changelog

All notable changes to atlOS are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow
[Semantic Versioning](https://semver.org/); while the version is 0.x, minor
versions may change interfaces. See [docs/releasing.md](docs/releasing.md).

## [Unreleased]

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

[Unreleased]: https://github.com/oguzhankynk/atlOS/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/oguzhankynk/atlOS/releases/tag/v0.1.0
