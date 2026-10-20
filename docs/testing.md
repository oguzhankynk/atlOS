# Self tests

The kernel has built-in tests in `kernel/tests/selftest.cpp`. A kernel built
with `ATLOS_SELFTEST=ON` (the `selftest` preset) runs them right after
the scheduler is set up instead of the keyboard demo, prints the results on the
serial port and halts with the result (`Core::haltWithStatus`).

How the result leaves the machine: the self test presets set
`ATLOS_DEBUG_EXIT_PORT=0xf4`, so the kernel writes its exit status (0 passed,
1 failed, 2 panic) to that I/O port before halting. The test scripts start QEMU
with an `isa-debug-exit` device on that port, which ends QEMU with
`(status << 1) | 1`. On a real machine, or with the option empty, nothing is
written: the kernel prints the results and halts, and the serial log is the
result.

## Running

```sh
scripts/build.sh selftest
# or
cmake --preset selftest && cmake --build --preset selftest --target selftest
ctest --preset selftest
```

```
[SELFTEST] start
[SELFTEST] PMM frames
...
[SELFTEST] done: 706 passed, 0 failed
SELFTEST PASSED
```

`scripts/run-selftest.sh <iso> [log] [disk.img]` does the QEMU part and can be
used on its own. With a disk image it attaches a copy as the primary ATA master
(the test disk, `scripts/make-disk-image.sh`; the `selftest` target passes
`build/selftest/disk.img`) and fails if the kernel then skips the disk tests.
After a passing run `scripts/check-test-disk.sh` checks the written copy on
the host: `fsck.fat -n` must find no errors and mtools must read the files the
kernel wrote; a failing check keeps the copy for a look.
Without one, or on a machine without that disk, the *ATA disk* group is
skipped. Its exit code is 0 when all checks pass, 1 when a check fails and 2
on a panic, crash or timeout (`SELFTEST_TIMEOUT`, default 60 s). A panic stops
the run at once, no need to wait for the timeout. The full serial log is
written to `build/selftest/selftest.log`; `scripts/symbolize.sh
build/selftest/kernel/atlOS.kernel build/selftest/selftest.log` turns a panic
backtrace in it into function names.

## What is covered

| Group | Checks |
|---|---|
| PMM blocks (isolated) | own 64 block instance: regions, reservations, exhaustion, bad frees, first fit, capacity clipping |
| PMM frames (isolated) | alignment, uniqueness, reuse, double free, pools give blocks back, the pool limit |
| PMM from a multiboot memory map | `populateFromMultiboot` on a fake multiboot2 info: rounding, kernel reservation |
| PMM (kernel instance) | the real manager hands out blocks and frames above the kernel, no leak |
| VMM | allocate/write/free, range reuse, `virtToPhys`, `mapPhysical`/`unmapPhysical` alias in a new page directory, no leak |
| Heap | sizes from 1 byte to 3 MB, no overlap, `krealloc`, `kcalloc` |
| Interrupts | the timer ticks; an interrupt preserves `rax`/`r12` |
| Uninitialized | `Cxx::Uninitialized<T>` constructs once, on demand, with the given arguments |
| PIT (fake controller) | the PIT driver against a fake `InterruptController`: period checks, attaches and unmasks IRQ 0 |
| Console (fake sink) | `Core::setConsole` routes printf to a sink and returns the previous one |
| VGA text console (RAM buffer) | clearing, text, newline, tab, backspace, color, line wrap, scrolling, on a buffer in RAM |
| Serial port (missing COM2) | a port that fails the loopback test reports it and drops writes instead of hanging (skipped on a machine with a COM2) |
| CPU | CPUID: max leaf, vendor string, SSE2/FXSR present, cache list bounded |
| Fault decoding | page fault and selector error codes, exception names, the vector numbers |
| TSS and double fault stack | `str` = 0x28, the descriptor is busy, IST1 is set and only vector 8 uses it |
| Backtrace | the rbp walk from a test function down to boot.S, the frame limit, broken chains |
| Thread stack | initial `rsp` is ABI aligned, stack page is freed on delete |
| Backtrace in a thread | the walk stops at the top of a thread's stack (rbp = 0) |
| Scheduler (isolated) | own schedulers on a fake tick source: round robin and exit, `sleep` tick by tick, `block`/`unblock`, removing a queued thread, instances do not affect each other |
| Scheduler sleep (kernel, PIT) | the kernel scheduler with real ticks: idling in `hlt`, a sleeping thread |
| Atomic, InterruptGuard, Spinlock | atomic operations and wrap around; guards nest and restore IF |
| WaitQueue (isolated) | FIFO wake order, `wakeOne` does not switch, `wakeAll` |
| Semaphore (isolated) | counting up to `max`, hand off to a waiter, `give` from interrupt context |
| Semaphore producer/consumer (isolated) | a 4 slot bounded buffer with two semaphores: order and sum |
| Mutex (isolated) | threads that yield while holding it run one after the other, ownership hand off |
| ConditionVariable (isolated) | Mesa semantics: the waiter checks again; woken waiters get the mutex back first |
| Scheduler idle keeps the interrupt state (kernel, PIT) | sleeping with interrupts off returns with them off |
| Scancode decoder (set 1, US) | make/break, Shift, Caps Lock, Ctrl, 0xE0 keys, Print Screen's fake shift, Pause and controller answers |
| Block devices and the MBR (RAM disk) | range checks, partition table parsing (bad signature, entries beyond the disk), a partition as its own device |
| FAT16 8.3 names | path component to the on disk 11 bytes and back |
| FAT16 (RAM disk) | a hand built volume: lookup (case, deleted entries, labels, LFN parts, ".."), reads across clusters out of order, broken chains (too short, a free cluster, out of range, a looping directory), the VFS on top, volumes that are not FAT16 |
| FAT16 writing (RAM disk) | create, append, overwrite, files over several clusters, both FAT copies equal, remove frees every cluster, mkdir/rmdir, a directory growing past a cluster, a full volume, a full root directory, the VFS |
| ATA disk (test disk) | IDENTIFY, the partition table, writing and reading back sector 1, `/disk0` as mounted at boot (`HELLO.TXT`, `DOCS/README.TXT`, a 22000 byte `BIG.TXT` line by line), then writing `KERNEL/NOTE.TXT` and `KERNEL/DATA.TXT` and creating/removing `KERNEL/TMP` |
| PS/2 keyboard and the character queue (fake interrupt controller) | IRQ 1 attached and unmasked, arrow keys as ANSI sequences, a reader blocked until characters arrive, a full queue drops new characters, sequences go in whole |
| VMM translate and mappings | `translate` tells physical 0 from unmapped, refuses non canonical addresses, covers the physmap (and no more); `forEachMapping` finds the kernel, the identity map and 64 GB of physmap |
| ELF64 parsing | a good two segment file; bad magic, 32-bit, wrong machine, not an executable, segments outside the file or the user range, sharing a page, the entry outside code, a broken header table |
| User programs (ring 3, test disk) | `HELLO.ELF` prints and exits with 0, `FAULT.ELF` ends with 142 (user page fault, no panic), `BADPTR.ELF` gets every bad pointer refused, missing and non ELF files, two programs at once, every page given back |
| Address spaces (a second one, user pages) | a new address space shares the kernel half and the identity map; user pages only in the user range, 2 MB aligned, zeroed, with the asked flags; built while inactive, seen after `activate()`; the destructor gives back pages and tables; kernel pages appear in every address space |
| Shell parser | words, quotes, limits; hex and decimal numbers, overflow |
| Shell line editor | echo, Backspace and DEL, `\r\n` as one Enter, Ctrl+C, skipped escape sequences, history (order, duplicates, full), the length limit |
| Shell commands (RAM disk) | every command on a FAT16 volume in RAM with the output captured: paths and `cd`, files, `echo >`/`>>`, `md`/`mw` on a test buffer, unmapped memory refused, `mem`, `vmmap`, `tasks` with a sleeping thread, errors |

## Crash test

Some code only runs when something goes wrong: the exception report and the
panic. The `crashtest` preset (`ATLOS_CRASHTEST=ON`) builds a kernel that,
after boot, reads an unmapped address on purpose (`kernel/tests/crashtest.cpp`).

```sh
scripts/build.sh crashtest
# or
cmake --preset crashtest && cmake --build --preset crashtest --target crashtest
ctest --preset crashtest
```

`scripts/run-crashtest.sh <iso> [log] [kernel]` boots it and checks the serial
log: the panic exit status, `EXCEPTION 14: Page fault`, the decoded
`CR2=0x00000000dead0000 (kernel read, page not present)`, a backtrace with at
least two frames and, with the kernel image, that `scripts/symbolize.sh`
resolves it to `CrashTest::readUnmapped` and `CrashTest::run`.

```
==== EXCEPTION 14: Page fault ====
error code 0x0
CR2=0x00000000dead0000 (kernel read, page not present)
...
==== KERNEL PANIC ====
Page fault
Backtrace (scripts/symbolize.sh shows the functions):
  #0 0xffffffff8041b6b3
  #1 0xffffffff8041b6f2
  ...
```

## Shell test

`scripts/build.sh shelltest` (`scripts/run-shelltest.py`) boots the normal
kernel with the test disk and COM1 on a TCP socket, waits for the prompt and
types commands the way a terminal does (`\r` for Enter, DEL for Backspace,
`ESC [ A` for Up, Ctrl+C), checking each answer: `ls`, `cat`, `cd`, writing
with `echo >`/`>>`, `rm`/`rmdir`, `mw`/`md`, `vmmap`, `tasks`, `run` of a user
program and of one that faults (the shell must still answer), the example of
`user/README.md` (`COUNT.ELF`), `scripts/symbolize.sh` on the user fault
report, and at the end
`fault`, whose kernel page fault report must appear. The serial log is
`build/default/shelltest.log`.

## Writing a test

```cpp
static void TestSomething()
{
    group("Something");          // printed as [SELFTEST] Something
    CHECK(value == expected);    // prints the expression and line on failure
}
```

Add the function to `SelfTest::run()`. Tests run in the boot flow with
interrupts enabled; a test that creates threads should wait for them to exit
(see `waitForExit`) and delete them before returning.
