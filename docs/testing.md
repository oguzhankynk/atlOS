# Self tests

The kernel has built-in tests in `kernel/tests/selftest.cpp`. A kernel built
with `ATLOS_SELFTEST=ON` (the `selftest` preset) runs them right after
the scheduler is set up instead of the demo threads, prints the results on the
serial port and exits QEMU through the `isa-debug-exit` device.

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
[SELFTEST] done: 122 passed, 0 failed
SELFTEST PASSED
```

`scripts/run-selftest.sh <iso> [log]` does the QEMU part and can be used on
its own. Its exit code is 0 when all checks pass, 1 when a check fails and 2
on a crash, panic or timeout (`SELFTEST_TIMEOUT`, default 60 s). The full
serial log is written to `build/selftest/selftest.log`.

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
| CPU | CPUID: max leaf, vendor string, SSE2/FXSR present, cache list bounded |
| Thread stack | initial `rsp` is ABI aligned, stack page is freed on delete |
| Scheduler (isolated) | own schedulers on a fake tick source: round robin and exit, `sleep` tick by tick, `block`/`unblock`, removing a queued thread, instances do not affect each other |
| Scheduler sleep (kernel, PIT) | the kernel scheduler with real ticks: idling in `hlt`, a sleeping thread |

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
