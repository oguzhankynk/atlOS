# Tutorial

atlOS from the first instruction GRUB runs to a program in ring 3, in the
order the kernel brings each piece up. Every chapter is a subsystem README
next to its code: what the problem is, how it could be solved, how atlOS
solves it, and where to read more. The code and the references go deeper.

Before you start: build and boot it ([building.md](building.md)), so you can
try each chapter in the shell. Some C++ and a little x86 assembly help; no OS
background is assumed.

## 0. A few words first

- **CPU** (processor): the chip that runs programs. It fetches an
  instruction from memory, executes it (add, compare, jump, load, store) and
  goes on to the next, billions of times a second. Its *registers* are a few
  small, fast storage places inside it.
- **Instruction set architecture (ISA)**: the list of instructions a CPU
  understands and how they are encoded. Programs are built for one ISA; ARM
  and RISC-V are other ISAs.
- **x86**: the ISA that began with Intel's 8086 (1978), a 16-bit CPU. Every
  later CPU of the family still runs its programs: the 80386 (1985) made it
  32-bit (*IA-32*). A PC still starts in that old 16-bit mode, and the boot
  code climbs up from there.
- **x86_64** (also *x86-64*, *AMD64*, *Intel 64*): the 64-bit extension,
  designed by AMD (first CPUs in 2003) and adopted by Intel: 64-bit registers and
  addresses, eight more registers, and *long mode*, the mode atlOS runs in.
- **Intel** and **AMD**: the two companies that make x86 CPUs. Both follow
  the same ISA, so the same kernel runs on either. Intel's *Software Developer's
  Manual* (SDM) is the reference for how the CPU behaves.
- **Operating system (OS)**: the program that owns the machine. It starts
  first, manages memory, the CPU's time and the devices, and lets other
  programs use them safely through a small interface (system calls). Its
  core, running with full privileges, is the **kernel**; atlOS is one.

## 1. Boot and long mode

GRUB, multiboot2, the boot page tables, the jump to 64-bit mode and the order
`kernelMain` constructs everything in.
[docs/architecture.md](architecture.md)

## 2. GDT and TSS

What is left of segmentation in 64-bit mode: rings, and the stacks the CPU
switches to. Try `gdt`.
[kernel/arch/x86_64/gdt](../kernel/arch/x86_64/gdt/README.md)

## 3. Interrupts and exceptions

The IDT, how a fault becomes a report with a backtrace, and the exception
table. Try `idt 14`, `irq`, `int 3`.
[kernel/arch/x86_64/idt](../kernel/arch/x86_64/idt/README.md)

## 4. Drivers: PIC, PIT, serial, keyboard, ATA

Talking to hardware through ports and memory, and the first interrupt
controller and timer. Try `in 0x21`, `irq`, `uptime`.
[kernel/drivers](../kernel/drivers/README.md)

## 5. ACPI and the APIC

Finding the firmware's tables, then the interrupt controller and timer that
replace the PIC and PIT. Try `acpi APIC`, `apic`.
[kernel/firmware/acpi](../kernel/firmware/acpi/README.md),
[kernel/drivers/apic](../kernel/drivers/apic/README.md)

## 6. Memory

Physical memory, page tables, and the heap that cuts pages into blocks (the
same code for the kernel and for user programs). Try `mem`, `vmmap`,
`pt 0xFFFFFFFF80000000`.
[kernel/mm](../kernel/mm/README.md),
[common/heap](../common/heap/README.md)

## 7. Threads and the scheduler

The context switch, a cooperative round robin scheduler, the idle thread and
CPU time. Try `tasks`, `sched`, `top`.
[kernel/sched](../kernel/sched/README.md)

## 8. Synchronization

Interrupt guards, wait queues, semaphores and mutexes on one CPU, and the
lost wakeup.
[kernel/sync](../kernel/sync/README.md)

## 9. File systems

A VFS and FAT16 on an ATA disk. Try `mount`, `ls /disk0`, `disk read 0`.
[kernel/fs](../kernel/fs/README.md)

## 10. User programs

ELF loading, ring 3, system calls, the start stack and a program's heap;
then writing a program yourself. Try `run /disk0/BIN/HELLO.ELF`,
`run -heap 8 /disk0/BIN/MALLOC.ELF`.
[kernel/proc](../kernel/proc/README.md),
[user](../user/README.md)

## 11. The kernel shell

How a command line is read and run, and how the shell looks at memory and
hardware without crashing the kernel.
[kernel/shell](../kernel/shell/README.md)

## 12. PCI and SMBIOS

Enumerating devices and reading what the firmware says about the machine.
Try `lspci`, `smbios 17`.
[kernel/drivers/pci](../kernel/drivers/pci/README.md),
[kernel/firmware/smbios](../kernel/firmware/smbios/README.md)

## Further reading

- [OSDev Wiki](https://wiki.osdev.org/): the starting point for every chapter
- Intel 64 and IA-32 Architectures Software Developer's Manual, Vol. 3
- *Operating Systems: Three Easy Pieces* (Arpaci-Dusseau), free online
- *xv6: a simple, Unix-like teaching operating system* (MIT 6.1810)
- [Self tests](testing.md), [code style and design](style.md)
