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
   1. constructs the `Drivers::VGATextConsole` and the COM1 `Drivers::SerialPort`
      and binds them as the console (`Core::setConsole`): `printf` goes to the
      screen first, to COM1 once it passed its loopback test
   2. banner, global C++ constructors (`cxx::callConstructors`)
   3. `IDT::init` (exception and IRQ vectors)
   4. checks the multiboot2 magic
   5. constructs the `Memory::PhysicalMemoryManager`, fills it from the
      multiboot memory map (`Memory::populateFromMultiboot`, reserves the
      kernel image), constructs the `Memory::AddressSpace` on top and binds it
      to the heap (see [memory.md](memory.md))
   6. constructs the `Fs::Vfs` and mounts the FAT16 partitions of the ATA
      disks (`Fs::mountAtaDisks`, see `kernel/fs/README.md`)
   7. constructs the `Drivers::PIC` (8259 remapped to vectors 0x20/0x28, all
      lines masked) and binds it to the ISR dispatcher
   8. constructs the `Drivers::PIT` on top of the PIC and enables interrupts
   9. constructs the `Threading::Scheduler` on the PIT (the boot flow becomes
      task 0, 1 ms tick)
   10. self tests (`ATLOS_SELFTEST` builds), the crash test (`ATLOS_CRASHTEST`)
       or the demo: lists the mounted disks, constructs the
       `Drivers::PS2Keyboard` and echoes what is typed

   `kernelMain` is the composition root: driver objects live in
   `Cxx::Uninitialized<T>` storage, are constructed there explicitly in
   dependency order and passed to their users by reference.

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
| `kernel_virtual_end .. 0xFFFFFFFFFFFFFFFF` | kernel heap, thread stacks, `mapPhysical` mappings (2 MB pages) |

## Interrupts

```
CPU → IDT vector → INT_<n> stub (arch/x86_64/idt/isr.S)
        push err_code (dummy 0 if the CPU does not push one), vector, rax..r15
        → intHandler(frame)            (arch/x86_64/idt/idt.cpp)
            exceptions 0..31: print the register dump and halt
            other vectors:   Handlers::interruptHandler(vector)
                               → InterruptController::endOfInterrupt(vector)
                               → registered handler(arg)
        ← pop registers, drop vector/err_code, iretq
```

Drivers do not talk to the PIC directly. They get a
`Drivers::InterruptController&` (`drivers/interrupt_controller.hpp`) and call
`attach(irq, fn, arg)` and `unmask(irq)`; the PIC is one implementation, an
APIC driver would be another. A periodic timer is a `Drivers::TickSource`
(`drivers/tick_source.hpp`), implemented by the PIT. The only place that
reaches the controller without being handed it is the ISR dispatcher, bound
once with `Handlers::setInterruptController`.

IRQ handlers run on the stack of whatever thread was interrupted; they must
not call into the scheduler or the heap.

All lines start masked; a line is enabled by the driver that uses it. Today
only IRQ 0 (PIT, scheduler tick) is enabled.

## Conventions

- Each subsystem is a directory under `kernel/` with headers next to their
  sources; includes are relative to `kernel/` (`#include <sched/thread.hpp>`).
- File names are lower case. Code lives in a namespace per subsystem
  (`Memory::PhysicalMemory`, `Scheduler`, `Threading`, `PIC`, ...); naming,
  formatting and the design direction are in [style.md](style.md).
- `libc/` is the freestanding C library (`libk`) that the kernel links; its
  public headers are in `libc/include`.
- Kernel code is built without exceptions, RTTI or the red zone.
- **No SIMD:** the kernel and libc are built with `-mno-mmx -mno-sse -mno-sse2`
  (`ATLOS_NO_SIMD_FLAGS`). Boot does not enable SSE and neither the context
  switch nor the ISRs save SIMD state, so the build fails if the linked image
  contains an MMX/SSE/AVX instruction (`cmake/check-no-simd.cmake`). For the
  same reason `printf` has no `%f`/`%e`/`%g`.
