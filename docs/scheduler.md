# Scheduler and threads

The scheduler (`kernel/sched/`) is **cooperative**: a thread runs until it
calls `yield`, `sleep`, `block` or returns. The PIT interrupt (1 ms) only
counts ticks; it never switches threads. Because of that, kernel code needs
no locks against other threads as long as it does not give up the CPU in the
middle of an update.

## Threads

```cpp
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>

void worker()
{
    for (int i = 0; i < 10; i++) {
        doSomething();
        Threading::yield(); // the running thread's scheduler
    }
} // returning ends the thread

auto* t = new Threading::Thread(worker); // on the caller's scheduler, queued right away
...
delete t; // owner frees it (any state but running)
```

- `Threading::currentTask()` is the task running now and
  `Threading::currentScheduler()` its scheduler; every switch updates them.
  `Threading::yield()`, `sleep(ms)` and `exitThread()` act on the running
  thread's scheduler. So a thread, a semaphore or a mutex is created without
  being handed a scheduler.
- `Thread(entry)` goes to the caller's scheduler with a stack from the kernel
  address space (`Memory::kernelAddressSpace()`); `Thread(scheduler, space,
  entry)` says both explicitly (the self tests use it for their own
  schedulers).
- Each thread gets its own 2 MB stack page, 16 byte aligned as the System V
  ABI requires; it is freed by the destructor.
- `Thread(..., false)` creates a thread without queueing it.
- Returning from the entry point calls `Scheduler::exit()`: the thread is
  taken out of the run queue and never runs again. The `Thread` object stays
  valid until its owner deletes it.
- The destructor removes a thread that is still queued. A thread must not
  delete itself.

## Scheduler API (`sched/scheduler.hpp`)

`Threading::Scheduler(tickSource)`: the flow that constructs it becomes task 0
(`kernelMain` for the kernel's scheduler) and the 1 ms tick is started on the
given `Drivers::TickSource` (the PIT), with the scheduler as the IRQ argument.

| Call | Effect |
|---|---|
| `yield()` | run the next runnable thread (round robin); returns at once if there is none |
| `sleep(ms)` | `Sleeping` for at least `ms` ticks |
| `block()` / `unblock(tcb)` | `Blocked` until another thread (or an IRQ handler) calls `unblock` |
| `exit()` | end the current thread |
| `current()` | TCB of the running thread |
| `ticks()` | ticks (ms) since construction |
| `taskCount()` | number of queued tasks, including task 0 |

Thread states: `Ready` and `Running` are runnable; `Sleeping`, `Blocked` and
`Terminated` are skipped.

Instances are independent: the self tests build their own schedulers on a fake
tick source, so sleeping is tested tick by tick without waiting for the PIT.

## How it works

- The run queue holds up to 16 TCB pointers. Entry 0 is the constructing
  flow, which has a TCB but no `Thread` object; it can sleep and block like
  any thread but must not exit.
- `schedule()` wakes sleepers whose `wakeTick` has passed, then picks the next
  runnable entry after the current one. If only the current thread is
  runnable, it keeps running. If nothing is runnable, it waits in `sti; hlt`
  until an interrupt changes that.
- `switchTask(from, to)` (`arch/x86_64/cpu/context.S`) saves all registers,
  `rflags` and `cr3` of the current thread and loads those of the next one.
  A new thread starts in `ThreadImpl::threadStartPoint` with the `ThreadImpl`
  pointer in `rdi`.
- An exited thread is removed from the queue while switching away from it;
  its stack is in use until `switchTask` returns, so only the owner frees it.
