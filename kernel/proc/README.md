# User programs (`kernel/proc/`)

(Writing a program: see `user/README.md`. This page is the kernel's side.)

How a file on the disk becomes a program running in **ring 3**: in its own
address space, unable to touch the kernel, talking to it only through
system calls, and ended by the kernel when it faults.

```
atlOS:/> run /disk0/BIN/HELLO.ELF
Hello from user mode!
running at 0x00000080000000d0
[pid 1 HELLO.ELF exited with 0]
atlOS:/> run /disk0/BIN/ASK.ELF
What is your name? Ada
Hello, Ada! I am process 2.
[pid 2 ASK.ELF exited with 0]
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
| Files of a program | **a small table of open files per process**, the index is the file descriptor (Unix) · handles to kernel objects (Windows) · no state: every call names the path and the offset |
| Keyboard input | **raw bytes to whoever reads, the program echoes** · a "cooked" line mode in the kernel (the Unix terminal: echo, Backspace, the line on Enter) |

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

The number in `rax`, the arguments in `rdi`, `rsi`, `rdx`, the result in
`rax`. A negative result is an error, with the Linux numbers (`-2` is
`-ENOENT`).

| rax | Call | Result |
|---|---|---|
| 0 | `exit(code)` | does not return |
| 1 | `write(fd, buffer, size)` | bytes written; `-EBADF`, `-EINVAL` (more than 4096 bytes), `-EFAULT`, `-EIO` |
| 2 | `read(fd, buffer, size)` | bytes read, 0 at the end of a file; `-EBADF`, `-EINVAL`, `-EFAULT`, `-EIO` |
| 3 | `open(path, flags)` | a file descriptor (3 and up); `-ENOENT`, `-EISDIR`, `-EMFILE`, `-EFAULT`, `-ENAMETOOLONG`, `-EINVAL` |
| 4 | `close(fd)` | 0; `-EBADF` |
| 5 | `yield()` | 0, after the other runnable threads had their turn |
| 6 | `sleep(ms)` | 0, at least `ms` milliseconds later |
| 7 | `getpid()` | the process id |
| 8 | `uptime()` | milliseconds since boot (the scheduler's ticks) |

An unknown number returns `-ENOSYS`. The gate is an interrupt gate with
DPL 3; every other gate has DPL 0, so `int $0x21` from a program is a #GP.

`Runtime::systemCall` is one `switch` over `rax`. A call that waits
(`read` from the keyboard, `sleep`, `yield`) waits on the program thread's
own kernel stack, inside the interrupt handler: the scheduler switches to
another thread and back later, then the handler returns to ring 3 as usual.
The interrupt flag is part of each thread's saved state, so the other
threads run with interrupts on while this one waits with them off.

### File descriptors

| fd | What |
|---|---|
| 0 | standard input: the keyboard and the serial terminal |
| 1, 2 | standard output and error: the screen and the serial terminal |
| 3 .. 10 | files from `open`: `Process::files[fd - 3]` |

An open file (`OpenFile`) is its path, its offset and how it was opened. The
VFS reads and writes by path and offset (`Vfs::read(path, offset, ...)`), so
`read` and `write` pass both and move the offset by what was transferred.

- `flags`: one of `O_RDONLY`, `O_WRONLY`, `O_RDWR`, plus `O_CREAT` (create
  it if missing), `O_TRUNC` (start empty: the FAT16 code has no truncate, so
  the file is removed and created again), `O_APPEND` (each write goes to
  the current end).
- Paths start at the root (`/disk0/...`): a program has no current
  directory. A relative path is `-ENOENT`.
- The path is copied out of the program byte by byte, each byte checked
  with `isUserAccessible`, up to 128 bytes with the terminating 0.
- At exit nothing has to be flushed (every write went to the disk), so the
  table goes away with the process.

### The keyboard: `read(0, ...)`

The shell and a program read the same `CharacterQueue` (the keyboard and
COM1 feed it). `run` waits for the program (`Runtime::wait`), so while it
runs the shell does not read and the keys go to the program.

`read(0, buffer, size)` sleeps until a key is there, then returns it and
the keys already queued behind it, at most `size`. The bytes are **raw**:
nothing is echoed, Backspace arrives as `0x7f` (DEL) or `0x08`, and a
program that wants a line editor writes one (`user/programs/ask.c`). Two
exceptions: Enter arrives as `\n` (a terminal sends `\r`), and **Ctrl+C**
(`0x03`) ends the program with 130, like a Unix shell's 128 + SIGINT.

### Waiting for a program

`Runtime::wait(pid)` takes the process's `done` semaphore, which
`exitCurrent` gives, then frees the program (`reap`) and returns its exit
code. The waiting thread sleeps meanwhile, so the CPU halts when the program
waits too (e.g. for a key).

**User pointers.** A program can pass any number as a pointer. Before
`write` reads the buffer, `AddressSpace::isUserAccessible` checks that every
byte is in the user range and mapped with the user bit at every table level.
Without that check a program could make the kernel print (or later, write)
kernel memory for it. (A real kernel also has to care that another thread
could unmap the memory while the kernel reads it; atlOS is cooperative.)

### Ending a program

- `exit(code)`: the runtime marks the process exited, prints
  `[pid 1 HELLO.ELF exited with 0]`, switches CR3 back to the kernel's tables
  gives `done` and ends the thread (`Scheduler::exit`).
- A breakpoint (`int3`) in ring 3 reports the registers and the program goes
  on.
- A fault in ring 3 (`frame.cs & 3 == 3`): the exception is reported as
  usual (`Page fault (in user mode)`, the decoded error code) and the program
  ends with `128 + vector`, e.g. 142 for a page fault. The kernel does not
  panic.
- A thread can not free its own kernel stack while running on it, so the
  exited program (thread, address space with all its pages and tables) is
  freed by another thread: `Runtime::wait` or `Runtime::reap`.

### The boundary global

The `int 0x80` handler and the ring 3 fault path are reached from the
assembly stubs, which can not be handed an object. `kernelMain` binds the
runtime once with `Process::setRuntime`, like `Handlers::setInterruptController`
(see `docs/style.md`).

### Deliberately not done

- No `syscall`/`sysret`: `int 0x80` goes the way every interrupt goes.
- **No preemption:** a program that never makes a system call keeps the CPU
  for good (the timer only counts ticks). Only `yield`, `sleep` and a `read`
  that waits let other threads run.
- No `lseek`, `stat`, directory listing, `unlink` or `rename` calls.
- No process object with several threads, no `fork`/`exec`, no arguments or
  environment.
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
  overlong pointers, a wrong fd and an unknown call are all refused),
  `ASK.ELF` with keys pushed into the queue first (a Backspace in them, then
  Ctrl+C: exit code 130), `CLOCK.ELF` (`uptime`, `sleep`, `yield`),
  `FILES.ELF` (`open`/`read`/`write`/`close`, `O_TRUNC`, `O_APPEND`, and the
  errors: a missing file, a relative path, a directory, a kernel pointer,
  the wrong access mode, closing twice, every descriptor in use) with the
  file it wrote read back through the VFS, files that are not programs, two
  programs at once at the same addresses, and that every page and table
  comes back. `scripts/check-test-disk.sh` reads `USER.TXT` with mtools too.
- The shell test (`scripts/run-shelltest.py`) runs `HELLO.ELF`, `FAULT.ELF`
  (the shell still answers afterwards), types a name into `ASK.ELF`, ends it
  once with Ctrl+C, runs `CLOCK.ELF` and `FILES.ELF` and `cat`s the file.

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
