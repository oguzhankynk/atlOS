# Interrupts and exceptions (`kernel/arch/x86_64/idt/`)

How the CPU gets from "something happened" (a division by zero, a page fault,
a timer tick) to a C++ handler, and back.

## What is it for?

Three things interrupt the normal flow of instructions:

| Kind | Source | Examples (vector) |
|---|---|---|
| **Exception** | the CPU itself, caused by the current instruction | divide error (0), invalid opcode (6), general protection (13), page fault (14) |
| **Hardware interrupt (IRQ)** | a device, through the interrupt controller | PIT timer (0x20), keyboard (0x21) |
| Software interrupt | an `int n` instruction | system calls in some OSes (`int 0x80`) |

For each of the 256 vectors the **IDT** (Interrupt Descriptor Table) holds a
*gate*: the address of the code to run and how to enter it. The CPU saves the
interrupted `rip`, `cs`, `rflags`, `rsp`, `ss` (and for some exceptions an
error code) on the stack and jumps there; `iretq` returns.

## How could it be designed?

| Question | Options |
|---|---|
| One entry point per vector, or one for all? | **a small assembly stub per vector** that pushes its number and jumps to a common path (almost everyone) · a single entry that reads the vector from the APIC (not possible on x86 for exceptions) |
| Which gate type? | *interrupt gate*: clears IF, no nested interrupts · *trap gate*: leaves IF as it was |
| Separate stacks? | handle everything on the current stack · **IST** stacks from the TSS for faults where the stack itself may be broken (double fault, NMI) |
| Dispatch in C | a big `switch` · a table of registered handlers · both |
| What to do on an exception | **report and stop** · recover (a page fault that maps the page: demand paging, copy-on-write) · kill the faulting process (user mode) |
| How to find the caller chain | **frame pointer walk** (`rbp` chain, needs `-fno-omit-frame-pointer`) · DWARF unwind tables (`.eh_frame`, works without frame pointers, needs an unwinder) · ORC (Linux: its own compact unwind format) |

## How atlOS does it

```
isr.S          INT_<n> stubs: push a dummy error code if the CPU did not,
               push n, push rax..r15, call intHandler(frame), pop, iretq
idt.cpp        builds the IDT: 0..31 trap gates, 32..255 interrupt gates,
               vector 8 (double fault) on IST1, 0x80 (system calls) DPL 3;
               intHandler: int 0x80 from ring 3 -> Process::Runtime::systemCall,
               vector < 32 from ring 3 -> report, end the program,
               vector < 32 in the kernel -> Fault::reportException (panic),
               everything else -> Handlers::interruptHandler(vector)
fault.cpp      decodes the error code (and CR2), prints the report and panics
               with a backtrace of the faulting code
handlers.cpp   table of (function, argument) per vector; sends the EOI
               through the bound InterruptController, then calls the handler
```

- **Stubs** are generated with `.rept` macros. The CPU pushes an error code
  only for vectors 8, 10–14, 17 and 21; the others push a `0` so every frame has
  the same layout (`InterruptFrame` in `cpu/cpu.hpp`).
- **Stack alignment**: the CPU aligns `rsp` to 16 before pushing its frame; 5
  qwords + error code + vector + 15 registers = 22 qwords, so `rsp` is still
  16-byte aligned at the `call`, as the ABI requires. No padding needed.
- **Trap gates for exceptions**, interrupt gates for IRQs: an IRQ handler runs
  with interrupts off, so it can not be interrupted by itself.
- **Dispatch table**: drivers register `(function, argument)` pairs through
  their `InterruptController` (`PIC::attach` writes vector `0x20 + irq`).
  The argument is how a handler gets its object (e.g. the scheduler) without a
  global.
- The only global here is the bound controller pointer
  (`Handlers::setInterruptController`): the assembly stubs can not be handed an
  object.

### Exception reports (`fault.cpp`)

An exception is fatal for now, so the job is to say as clearly as possible
*what* went wrong and *where*. The CPU gives three clues:

| Clue | Where | Meaning |
|---|---|---|
| vector | pushed by the stub | which exception (14 = page fault, 13 = #GP, 8 = double fault) |
| error code | pushed by the CPU for 8, 10–14, 17, 21 | details, its format depends on the vector |
| CR2 | control register | page fault only: the address that was accessed |

The decoders are pure functions of the error code, so the self tests check
them without faulting:

```
Page fault error code                 Selector error code (#TS #NP #SS #GP)
bit 0  P    0 not present / 1 protection   bit 0     EXT  during an external event
bit 1  W/R  0 read / 1 write               bit 1     IDT  index is an IDT vector
bit 2  U/S  0 kernel / 1 user mode         bit 2     TI   0 GDT / 1 LDT (if not IDT)
bit 3  RSVD reserved bit in a table entry  bits 3-15      descriptor index
bit 4  I/D  instruction fetch              (0 = not caused by a selector)
```

A report:

```
==== EXCEPTION 14: Page fault ====
error code 0x0
CR2=0x00000000dead0000 (kernel read, page not present)
RAX=... (registers)
==== KERNEL PANIC ====
Page fault
Backtrace (scripts/symbolize.sh shows the functions):
  #0 0xffffffff8041b6b3      <- the faulting instruction (rip)
  #1 0xffffffff8041b6f2      <- return addresses up the frame chain
```

### Backtrace (`cpu/backtrace.cpp`)

With frame pointers every function starts with `push %rbp; mov %rsp, %rbp`,
so the saved `rbp` values form a linked list through the stack:

```
higher addresses
  | return address into kernelMain |
  | saved rbp (kernelMain's)       | <-+
  |  ... locals of run ...         |   |
  | return address into run        |   |
  | saved rbp (run's)              | --+  <- rbp inside readUnmapped
lower addresses
```

`Backtrace::collect` follows it and stops at `rbp = 0` (`boot.S` and every new
thread start with it), at a zero return address, or when the next frame is not
higher up the stack (a broken chain). The kernel only prints addresses; it has
no symbol table at run time. `scripts/symbolize.sh <atlOS.kernel> <log>` runs
`addr2line` on them:

```
#0 0xffffffff8041b6b3 CrashTest::readUnmapped() at kernel/tests/crashtest.cpp:18
#1 0xffffffff8041b6f2 CrashTest::run() at kernel/tests/crashtest.cpp:24
#2 0xffffffff804131c0 kernelMain at kernel/core/main.cpp:154
```

### Panic and halting (`core/panic.cpp`, `core/halt.cpp`)

Exceptions and `kernelPanic(message)` take the same path: `cli`, message,
backtrace, `Core::haltWithStatus(Panic)`. A second panic while the first one
reports (say, the backtrace walks into unmapped memory) only prints a line and
halts, instead of reporting forever. `haltWithStatus` ends in a `cli; hlt`
loop; test builds first write the status to `ATLOS_DEBUG_EXIT_PORT` so the
test machine can stop at once (see `docs/testing.md`).

### Faults that are expected: the exception table

Some kernel code may fault on purpose: reading an MSR the CPU may not have
(`readMsr`), reading memory that may not be mapped (`readQword`). Such an
instruction gets an entry in the **exception table**: its address and the
address to resume at, in the section `.extable`, which the linker collects
between `__extable_start` and `__extable_end`.

```
msr.S:   1: rdmsr            ...   2: xor %eax, %eax; ret      .extable: .quad 1b, 2b
```

When an exception arrives in ring 0, `intHandler` first calls
`Fault::fixup(frame)`: if `frame.rip` is in the table, it is replaced by the
resume address and the handler returns, so `iretq` continues there and the
function returns false. Anything else is a real bug and panics. Linux does
the same (`__ex_table`, used by `copy_from_user`).

### Counting interrupts

`intHandler` counts every vector on entry (`IDT::interruptCount`, an
atomic add, since trap gates let interrupts nest); the shell's `irq` lists
the counts.

### Pitfalls (and the tests that guard them)

- **The stub must give back every register unchanged.** After the call it
  drops exactly the vector and the error code (`addq $16, %rsp`) and pops
  the 15 registers it pushed; touching `rax` or any other register on the way
  corrupts the interrupted code, often only once the timer interrupt fires.
  The *Interrupts* self test puts known values in `rax` and `r12`, waits for
  an interrupt with `hlt` and checks they survived.
- **`InterruptFrame` follows the push order of `isr.S`.** The stub pushes
  `rax` first and `r15` last, so `r15` is at the lowest address and the
  struct starts with it; `rsi` is pushed before `rdi`, so `rdi` comes first
  in the struct. System calls read their arguments from the frame (`rdi`,
  `rsi`, `rdx`): the user program tests fail if the order is wrong.
- **`ExternalInterrupts` values are the vector numbers.** *Fault decoding*
  checks a few of them (10, 14, 21).

### Deliberately not done

- Exceptions in the kernel report and halt; nothing is recovered (no demand
  paging). In ring 3 they end the program (`kernel/proc/README.md`).
- Only the double fault has its own stack. NMI and machine check would want
  IST stacks too once they can happen in the middle of anything.
- No symbol names at run time; the kernel prints addresses only.
- Reserved vectors (9, 15, 22–31) have no stub.

## Flow: one interrupt

```mermaid
sequenceDiagram
    participant Code as interrupted code
    participant CPU
    participant Stub as INT_n (isr.S)
    participant C as intHandler (idt.cpp)
    participant H as Handlers (handlers.cpp)
    Code->>CPU: (IRQ arrives / instruction faults)
    CPU->>CPU: push ss, rsp, rflags, cs, rip [, error code]
    CPU->>Stub: jump via IDT[n]
    Stub->>Stub: push 0 (if no error code), push n, push rax..r15
    Stub->>C: call intHandler(frame)
    alt n < 32 (exception)
        C->>C: Fault::reportException: decode, registers
        C->>C: kernelPanicFrom(rip, rbp): backtrace, cli; hlt
    else IRQ / other
        C->>H: interruptHandler(n)
        H->>H: controller->endOfInterrupt(n)
        H->>H: handler(arg)
    end
    C-->>Stub: return
    Stub->>Stub: pop r15..rax, drop n and error code
    Stub->>Code: iretq
```

## Tests

*Interrupts*: the timer ticks, and an interrupt preserves `rax`/`r12`.
*PIT (fake interrupt controller)* covers registration through the controller.
*Fault decoding* checks the error code decoders and exception names, *TSS and
double fault stack* the IST setup, *Backtrace* and *Backtrace in a thread* the
frame walk.

The report itself only runs when something breaks, so the **crash test**
(`scripts/build.sh crashtest`, also in CI) boots a kernel that reads an
unmapped address and checks the serial log for the decoded page fault and a
backtrace that symbolizes to the right functions.

## References

- OSDev Wiki: [Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table),
  [Interrupt Service Routines](https://wiki.osdev.org/Interrupt_Service_Routines),
  [Exceptions](https://wiki.osdev.org/Exceptions), [Task State Segment](https://wiki.osdev.org/Task_State_Segment),
  [Stack Trace](https://wiki.osdev.org/Stack_Trace)
- Intel SDM Vol. 3A, chapter "Interrupt and Exception Handling" (gate types,
  stack frame, error codes, IST; "Interrupt 14—Page-Fault Exception")
- System V AMD64 ABI: the frame pointer convention
- xv6 book (MIT 6.1810): chapter "Traps and system calls"
- *Operating Systems: Three Easy Pieces*: "Limited Direct Execution"
