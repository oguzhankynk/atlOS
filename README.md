# atlOS

<img src="docs/images/logo.jpg" alt="atlOS logo" width="160" align="right">

atlOS is an experimental x86_64 operating system written in C++17 for learning
and research. It boots with GRUB (multiboot2) into a higher half kernel and is
small enough to read end to end.

## Status

Working:

- 64-bit higher half kernel (loaded at 4 MB, mapped at `0xFFFFFFFF80000000` + physical), booted by GRUB2
- GDT, IDT, exception reporting, 8259 PIC, PIT (1 ms tick)
- Physical memory manager (2 MB blocks + 4 KB frames for page tables)
- Virtual memory manager with 2 MB pages and recursive page table mapping
- Kernel heap (`kmalloc`/`kfree`, based on liballoc 1.1)
- Cooperative round robin scheduler: threads, `yield`, `sleep`, `block`/`unblock`
- Serial (COM1) and VGA text output, a small freestanding libc (`libk`)
- In-kernel self tests run headless in QEMU

In the tree but not wired up yet: ATA, FAT16 and VFS code, the keyboard driver,
mutex and semaphore. The CPUID helpers are built and tested but not called at
boot. There is no user mode, no system calls and no shell; atlOS runs on a
single CPU.

## Quick start

You need an `x86_64-elf` GCC cross compiler on `PATH`, CMake 3.22+, Ninja,
`grub-mkrescue` (with xorriso) and QEMU. See [docs/building.md](docs/building.md)
for building the toolchain.

```sh
scripts/build.sh            # libc, kernel and build/default/atlOS.iso
scripts/build.sh run        # boot in QEMU, serial output on the terminal
scripts/build.sh selftest   # build and run the self tests
```

The same with CMake directly:

```sh
cmake --preset default
cmake --build --preset default --target run
```

## Source layout

```
kernel/
  arch/x86_64/   boot, crt, cpu (context switch, CPUID), gdt, idt (+ISR stubs), io ports, mmu
  core/          kernel entry (main.cpp), panic, logger, banner
  cxx/           C++ runtime support (global constructors, new/delete)
  mm/            pmm, vmm, heap (liballoc)
  sched/         scheduler, threads
  sync/          spinlock, mutex, semaphore
  drivers/       pic, pit, serial, tty/vga, keyboard, disk/ (ATA)
  dev/           device manager
  fs/            vfs, fat16
  firmware/      bios, smbios
  lib/           small header-only helpers
  tests/         in-kernel self tests
libc/            freestanding C library (libk) used by the kernel
cmake/           cross toolchain file
scripts/         build.sh, run-selftest.sh
tools/           grub.cfg, FAT disk image scripts
docs/            documentation
```

Headers live next to their sources; includes are relative to `kernel/`, for
example `#include <mm/vmm.hpp>`.

## Documentation

- [Building and running](docs/building.md)
- [Architecture and boot flow](docs/architecture.md)
- [Memory management](docs/memory.md)
- [Scheduler and threads](docs/scheduler.md)
- [Self tests](docs/testing.md)
- [Code style and design](docs/style.md)

API documentation can be generated with Doxygen: `scripts/build.sh docs`
(output in `build/docs/html`).

## License

MIT, see [LICENSE](LICENSE).
