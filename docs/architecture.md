# Architecture

## Boot flow

1. **GRUB** loads `atlOS.kernel` (multiboot2, see `kernel/arch/x86_64/boot/`)
   at physical `0x400000` and jumps to `_start` in 32-bit protected mode.
2. **`boot.S`** builds the boot page tables and enters long mode:
   - `PML4[0] → low_pdpt → low_page_directory_table`: identity map of
     `0 .. kernel_physical_end` with 2 MB pages
   - `PML4[511] → high_pdpt[510] → high_page_directory_table`: the same
     physical range at `0xFFFFFFFF80000000` (the higher half)
   - enables PAE/PSE, `EFER.LME` and paging, loads the GDT and far jumps to
     `_start64`, which calls `kernelMain(mbInfo, mbMagic)`.
3. **`kernelMain`** (`kernel/core/main.cpp`):
   1. VGA text terminal, then COM1 serial; `printf` goes to serial from here on
   2. banner, global C++ constructors (`cxx::callConstructors`)
   3. `IDT::init` (exception and IRQ vectors)
   4. checks the multiboot2 magic
   5. `Memory::init`: PMM, recursive mapping, free memory from the multiboot
      memory map, reserves the kernel image (see [memory.md](memory.md))
   6. `DeviceManager::init` (currently empty)
   7. `PIC::init` (8259 remapped to vectors 0x20/0x28, enables interrupts)
   8. `Scheduler::init` (boot flow becomes task 0, PIT at 1 ms)
   9. self tests (`ATLOS_SELFTEST` builds) or the demo threads A/B

The linker script (`kernel/arch/x86_64/linker.ld`) places the boot sections
at their physical address and everything else at
`KERNEL_VIRTUAL_LOAD_OFFSET + physical`. `kernel_physical_end` and
`kernel_virtual_end` are rounded up to 2 MB.

## Address space

| Virtual range | Contents |
|---|---|
| `0x0 .. kernel_physical_end` | identity map from boot (multiboot info, boot tables) |
| `0xFFFFFF0000000000` (PML4[510]) | recursive mapping of the page tables |
| `0xFFFFFFFF80000000 .. kernel_virtual_end` | kernel image |
| `kernel_virtual_end .. 0xFFFFFFFFFFFFFFFF` | kernel heap, thread stacks, `Memory::map` mappings (2 MB pages) |

## Interrupts

```
CPU → IDT vector → INT_<n> stub (arch/x86_64/idt/isr.S)
        push err_code (dummy 0 if the CPU does not push one), vector, rax..r15
        → intHandler(frame)            (arch/x86_64/idt/idt.cpp)
            exceptions 0..31: print the register dump and halt
            other vectors:   Handlers::interruptHandler(vector)
                               → EOI to the PIC for IRQs 32..47
                               → registered handler(arg)
        ← pop registers, drop vector/err_code, iretq
```

Handlers are registered with `PIC::addIrqHandler(irq, fn, arg)` or
`Handlers::addInterruptHandler(vector, fn, arg)`. IRQ handlers run on the
stack of whatever thread was interrupted; they must not call into the
scheduler or the heap.

Enabled IRQs: 0 (PIT, scheduler tick) and 1 (keyboard; no driver is
initialized yet, so key presses only reach the default handler).

## Conventions

- Each subsystem is a directory under `kernel/` with headers next to their
  sources; includes are relative to `kernel/` (`#include <sched/thread.hpp>`).
- File names are lower case. Code lives in a namespace per subsystem
  (`Memory::PhysicalMemory`, `Scheduler`, `Threading`, `PIC`, ...).
- `libc/` is the freestanding C library (`libk`) that the kernel links; its
  public headers are in `libc/include`.
- Kernel code is built without exceptions, RTTI or the red zone.
- **No SIMD:** the kernel and libc are built with `-mno-mmx -mno-sse -mno-sse2`
  (`ATLOS_NO_SIMD_FLAGS`). Boot does not enable SSE and neither the context
  switch nor the ISRs save SIMD state, so the build fails if the linked image
  contains an MMX/SSE/AVX instruction (`cmake/check-no-simd.cmake`). For the
  same reason `printf` has no `%f`/`%e`/`%g`.
