# Self tests

The kernel has built-in tests in `kernel/tests/selftest.cpp`. A kernel built
with `ATLOS_SELFTEST=ON` (the `selftest` preset) runs them right after
the scheduler is set up instead of the demo threads, prints the results on the
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
[SELFTEST] done: 175 passed, 0 failed
SELFTEST PASSED
```

`scripts/run-selftest.sh <iso> [log]` does the QEMU part and can be used on
its own. Its exit code is 0 when all checks pass, 1 when a check fails and 2
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
| VGA text console (RAM buffer) | clearing, text, newline, tab, color, line wrap, scrolling, on a buffer in RAM |
| Serial port (missing COM2) | a port that fails the loopback test reports it and drops writes instead of hanging (skipped on a machine with a COM2) |
| CPU | CPUID: max leaf, vendor string, SSE2/FXSR present, cache list bounded |
| Fault decoding | page fault and selector error codes, exception names, the vector numbers |
| TSS and double fault stack | `str` = 0x28, the descriptor is busy, IST1 is set and only vector 8 uses it |
| Backtrace | the rbp walk from a test function down to boot.S, the frame limit, broken chains |
| Thread stack | initial `rsp` is ABI aligned, stack page is freed on delete |
| Backtrace in a thread | the walk stops at the top of a thread's stack (rbp = 0) |
| Scheduler (isolated) | own schedulers on a fake tick source: round robin and exit, `sleep` tick by tick, `block`/`unblock`, removing a queued thread, instances do not affect each other |
| Scheduler sleep (kernel, PIT) | the kernel scheduler with real ticks: idling in `hlt`, a sleeping thread |

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
