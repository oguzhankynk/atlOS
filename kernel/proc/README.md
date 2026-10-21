# User programs (`kernel/proc/`)

(Writing a program: see `user/README.md`. This page is the kernel's side.)

How a file on the disk becomes a program running in **ring 3**: in its own
address space, unable to touch the kernel, talking to it only through
system calls, and ended by the kernel when it faults.

```
atlOS:/> run /disk0/BIN/HELLO.ELF
started pid 1
atlOS:/> Hello from user mode!
running at 0x00000080000000d0
[pid 1 HELLO.ELF exited with 0]
```

## What is it for?

So far everything ran in ring 0: any bug could overwrite any memory, any
loop could stop the machine. A **user program** runs with the CPU's
privilege level 3, where:

- pages without the *user* bit fault (the whole kernel half, the kernel's
  identity map),
- privileged instructions (`cli`, `hlt`, `mov cr3`, `in`/`out`) raise #GP,
- interrupt gates with DPL 0 can not be entered with `int`.

The kernel gets control back on every interrupt, exception and system call:
the CPU switches to the kernel stack named in the TSS (`rsp0`) and to
ring 0 code through the IDT. A fault in a program ends that program only.

## How could it be designed?

| Question | Options |
|---|---|
| Executable format | **ELF** (Unix, Linux) · flat binary (`objcopy -O binary`) · PE (Windows) |
| Address spaces | **one per program**, kernel half shared · one for all, programs at different addresses (no isolation between them) |
| Entering ring 3 | **`iretq` with a made up frame** · `sysret` |
| System calls | **`int 0x80`** (a software interrupt: simple, slower) · `syscall`/`sysret` (MSRs, fast; Linux x86_64) · `sysenter` (32-bit) |
| Who runs the program | **a kernel thread that drops to ring 3**, its kernel stack is the program's kernel stack · a separate process object owning several threads |
| Ending a program | **exit system call or a fault**; the thread ends, its memory is freed by someone else |

## How atlOS does it

### Building a program (`user/`)

Static ELF64 executables, built with the same cross compiler, no libc:
`user/lib/crt0.S` (`_start`: call `main`, pass its result to `exit`),
`user/lib/atlos.h` (the system call wrappers), `user/user.ld`. The linker
script puts the code at `userBase` = `0x0000008000000000` (`PML4[1]`) and the
data in the next 2 MB page, with `-z max-page-size=0x200000`: protection is
per 2 MB page, so code (read + execute) and data (read + write) must not
share one. The disk image gets them as `/BIN/HELLO.ELF`, `/BIN/FAULT.ELF`,
`/BIN/BADPTR.ELF`.

```
$ readelf -l build/default/user/hello.elf
  LOAD  offset 0x000  vaddr 0x0000008000000000  filesz 0x1b8  memsz 0x1b8  R E
  LOAD  offset 0x1c0  vaddr 0x00000080002001c0  filesz 0x017  memsz 0x01c  RW
```

### Loading (`elf.cpp`, `process.cpp`)

`parseElf` only reads numbers out of the file and checks them: the ELF
magic, 64-bit little endian, an executable for x86_64, program headers
inside the file, every `PT_LOAD` segment inside the file and inside the user
range, no two segments in one 2 MB page, the entry point inside an
executable segment. The file is input from outside; nothing is trusted.

`Runtime::spawn(path)` then:

1. creates an `AddressSpace(pmm, kernel)`: the kernel half and the identity
   map shared, the user range empty;
2. for every segment allocates its 2 MB pages (zeroed: a program must never
   see what a page held before; the zeros are also its `.bss`), user and,
   for data, writable; copies the file bytes in **through the physmap**,
   because the new address space is not active;
3. maps one writable page for the stack, its top at `0x00007FFFFFFFFFF0`;
4. creates a kernel `Thread` whose saved `cr3` is the new address space and
   whose `owner` is the process, and queues it.

### Entering ring 3 (`arch/x86_64/cpu/usermode.S`)

The thread's first code is `Runtime::threadEntry`, which calls
`enterUserMode(entry, stackTop)`: it pushes the frame `iretq` expects, as if
an interrupt had come from ring 3, clears every register and returns into
it:

```
push 0x23          ss      user data segment (GDT 4) | RPL 3
push stackTop      rsp
push 0x202         rflags  IF set: interrupts stay on
push 0x1b          cs      user code segment (GDT 3) | RPL 3
push entry         rip
iretq
```

### Back into the kernel

When an interrupt, an exception or `int 0x80` arrives in ring 3, the CPU
loads `rsp` from **`TSS.rsp0`** before pushing anything: the top of the
program thread's kernel stack. The scheduler sets `rsp0` on every switch
(`GDT::setKernelStack`), so each program thread lands on its own stack.

```mermaid
sequenceDiagram
    participant P as program (ring 3)
    participant CPU
    participant S as isr.S stub
    participant I as intHandler
    participant R as Runtime
    P->>CPU: int $0x80 (rax = 1 write, rdi = 1, rsi = buffer, rdx = size)
    CPU->>CPU: gate 0x80 has DPL 3: allowed; rsp = TSS.rsp0
    CPU->>S: push ss, rsp, rflags, cs, rip
    S->>I: push registers, call intHandler(frame)
    I->>R: systemCall(frame)
    R->>R: buffer in the user range and mapped User? print it
    R-->>I: frame.rax = size (or -EFAULT ...)
    I-->>S: return
    S->>P: pop registers (rax = the result), iretq to ring 3
```

### System calls (`int 0x80`)

| rax | Call | Arguments | Result |
|---|---|---|---|
| 0 | `exit(code)` | rdi | does not return |
| 1 | `write(fd, buffer, size)` | rdi, rsi, rdx | `size`, or `-EBADF` (fd not 1), `-EINVAL` (more than 4096 bytes), `-EFAULT` |

An unknown number returns `-ENOSYS`. The gate is an interrupt gate with
DPL 3; every other gate has DPL 0, so `int $0x21` from a program is a #GP.

**User pointers.** A program can pass any number as a pointer. Before
`write` reads the buffer, `AddressSpace::isUserAccessible` checks that every
byte is in the user range and mapped with the user bit at every table level.
Without that check a program could make the kernel print (or later, write)
kernel memory for it. (A real kernel also has to care that another thread
could unmap the memory while the kernel reads it; atlOS is cooperative.)

### Ending a program

- `exit(code)`: the runtime marks the process exited, prints
  `[pid 1 HELLO.ELF exited with 0]`, switches CR3 back to the kernel's tables
  and ends the thread (`Scheduler::exit`).
- A breakpoint (`int3`) in ring 3 reports the registers and the program goes
  on.
- A fault in ring 3 (`frame.cs & 3 == 3`): the exception is reported as
  usual (`Page fault (in user mode)`, the decoded error code) and the program
  ends with `128 + vector`, e.g. 142 for a page fault. The kernel does not
  panic.
- A thread can not free its own kernel stack while running on it, so
  `Runtime::reap()` frees exited programs (thread, address space with all its
  pages and tables) later: the shell calls it before every prompt.

### The boundary global

The `int 0x80` handler and the ring 3 fault path are reached from the
assembly stubs, which can not be handed an object. `kernelMain` binds the
runtime once with `Process::setRuntime`, like `Handlers::setInterruptController`
(see `docs/style.md`).

### Deliberately not done

- No `syscall`/`sysret`, no `read`, `yield`, `sleep` or file calls yet.
- **No preemption:** a program that never makes a system call keeps the CPU
  for good (the timer only counts ticks). Every system call returns to the
  same program; nothing switches away from it before `exit`.
- No process object with several threads, no `fork`/`exec`, no arguments or
  environment, no `wait`.
- No NX bit: data pages are executable too.
- No guard page under the stack (it is the only page there; running off it
  faults anyway).

## Tests

- *ELF64 parsing*: made up headers: a good two segment file, then each
  thing the loader must refuse.
- *Address spaces*: a second address space, user pages, flags, `activate`.
- *User programs (ring 3, test disk)*: runs `HELLO.ELF` (output, exit code
  0), `FAULT.ELF` (writes to address 0: a protection violation, exit code
  142, no kernel panic), `BADPTR.ELF` (kernel, kernel only, unmapped and
  overlong pointers, a wrong fd and an unknown call are all refused), files
  that are not programs, two programs at once at the same addresses, and
  that every page and table comes back.
- The shell test (`scripts/run-shelltest.py`) runs `HELLO.ELF` and
  `FAULT.ELF` from the shell and checks the shell still answers afterwards.

## References

- Intel SDM Vol. 3A: "Privilege Levels", "Calls to Interrupt or Exception
  Handlers" (stack switch, the TSS), "IRET", "Paging" (the U/S bit)
- System V ABI, x86-64 supplement: process initialization (the stack at
  entry); ELF-64 Object File Format
- OSDev Wiki: [Getting to Ring 3](https://wiki.osdev.org/Getting_to_Ring_3),
  [ELF](https://wiki.osdev.org/ELF), [System Calls](https://wiki.osdev.org/System_Calls)
- xv6 book (MIT 6.1810): chapters "Traps and system calls", "Page tables"
  (`exec`)
- *Operating Systems: Three Easy Pieces*: "Limited Direct Execution"
