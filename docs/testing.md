# Self tests

The kernel has built-in tests in `kernel/tests/selftest.cpp`. A kernel built
with `ATLOS_SELFTEST=ON` (the `selftest` preset) runs them right after
`Scheduler::init` instead of the demo threads, prints the results on the
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
[SELFTEST] done: 71 passed, 0 failed
SELFTEST PASSED
```

`scripts/run-selftest.sh <iso> [log]` does the QEMU part and can be used on
its own. Its exit code is 0 when all checks pass, 1 when a check fails and 2
on a crash, panic or timeout (`SELFTEST_TIMEOUT`, default 60 s). The full
serial log is written to `build-selftest/selftest.log`.

## What is covered

| Group | Checks |
|---|---|
| PMM frames | alignment, no overlap with the kernel, uniqueness, reuse, double free, pools give blocks back |
| PMM blocks | every free block can be allocated, then `nullptr`; double free; contiguous allocation; no leak |
| VMM | allocate/write/free, range reuse, `virtToPhysAddr`, `map`/`unmap` alias in a new page directory, no leak |
| Heap | sizes from 1 byte to 3 MB, no overlap, `krealloc`, `kcalloc` |
| Interrupts | the timer ticks; an interrupt preserves `rax`/`r12` |
| CPU | CPUID: max leaf, vendor string, SSE2/FXSR present, cache list bounded |
| Thread stack | initial `rsp` is ABI aligned, stack page is freed on delete |
| Scheduler | round robin and exit, `sleep`, `block`/`unblock`, removing a queued thread |

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
