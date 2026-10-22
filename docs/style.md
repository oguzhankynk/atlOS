# Code style and design

## Format

atlOS uses the [WebKit code style](https://webkit.org/code-style-guidelines/),
enforced by clang-format (`.clang-format`, `BasedOnStyle: WebKit`):

- 4 spaces, no tabs, no column limit
- function braces on their own line, other braces on the same line
- no indentation inside the outermost namespace
- `T* p` / `T& r`
- `#pragma once` instead of include guards

```sh
scripts/build.sh format         # rewrite all kernel and libc sources
scripts/build.sh format-check   # fail if a file is not formatted
```

## Naming

| What | Style | Example |
|---|---|---|
| types, namespaces, enum values | PascalCase | `PhysicalMemory`, `ThreadState::Running` |
| functions, methods | camelCase | `allocateBlocks`, `addTask` |
| variables, parameters, struct fields | camelCase | `pageIndex`, `lbaFirstSector` |
| class data members | `m_` + camelCase | `m_name` |
| static and namespace scope variables | `s_` + camelCase | `s_readyQueue` |
| `constexpr` constants | camelCase | `maxTasks`, `pageSize2Mb` |
| macros | UPPER_CASE | `KERNEL_VIRTUAL_LOAD_OFFSET` |

Acronyms in type names stay upper case (`CPUInfo`, `VGAColor`).

Exceptions, all excluded from the naming check:

- names shared with assembly or the linker script (`kernelMain`,
  `switchTask`, `gdt_ptr`, `pml4`, `kernel_physical_end`, ...)
- reserved and ABI names (`__cxa_*`, `__ctors_start`, `_text_start`, ...)
- libc (standard C names), `kernel/mm/liballoc` (third party) and the
  multiboot headers in `kernel/arch/x86_64/boot` (copied from the spec)

## clang-tidy

`.clang-tidy` enables `bugprone-*`, `clang-analyzer-*`, `performance-*` and
`readability-identifier-naming` (the naming table above). Disabled on purpose:
`bugprone-easily-swappable-parameters` (noise for `(addr, size)` APIs),
`bugprone-reserved-identifier` (the C++ runtime has to define `__cxa_*`),
`performance-no-int-to-ptr` (page table code casts addresses by design);
`modernize-*` and `cppcoreguidelines-*` are not used because they fight the
raw pointers and casts a kernel needs.

```sh
scripts/build.sh tidy           # kernel sources, uses build/default/compile_commands.json
```

The kernel is clean: new warnings should be fixed, not suppressed. Code inside
`#ifdef ATLOS_SELFTEST` is only seen when running tidy on `build/selftest`.

## Third party code

`kernel/mm/liballoc` (liballoc 1.1) and `libc/stdio/printf.cpp`
(mpaland/printf) are kept as upstream: no reformatting, no renaming.

## Subsystem READMEs

Each subsystem directory has a `README.md` with the same sections:

1. **What is it for?** the problem, without assuming OS background
2. **How could it be designed?** the usual approaches, with pros, cons and
   who uses them (Linux, xv6, ...)
3. **How atlOS does it** and why; what is deliberately left out
4. **Flow** diagrams (mermaid, rendered by GitHub)
5. **Tests** that cover it, and **references** (OSDev Wiki, Intel SDM, xv6,
   OSTEP)

A change to a subsystem updates its README in the same commit series.

## Design

Agreed direction, introduced subsystem by subsystem. Done: the interrupt
controllers and timers (`Drivers::ApicInterruptController`, `Drivers::PIC`,
`Drivers::ApicTimer`, `Drivers::PIT` behind `InterruptController` /
`TickSource`), the physical memory manager
(`Memory::PhysicalMemoryManager`), the kernel address space
(`Memory::AddressSpace`) and the scheduler (`Threading::Scheduler`; threads
get it as their entry point argument), all built by `kernelMain` in
`Cxx::Uninitialized` storage, and the console (`Drivers::SerialPort` and
`Drivers::VGATextConsole` behind `CharacterSink`, bound to printf with
`Core::setConsole`). Not done: the heap, whose state lives in liballoc
(third party) behind the `setHeapAddressSpace` boundary. `logMsg` keeps no
state and stays a function over printf.

- **No singletons.** Each subsystem is a class that owns its state
  (`PhysicalMemoryManager`, `AddressSpace`, `Heap`, `Scheduler`, drivers);
  there are no `instance()` accessors and no lazily created globals.
- **Dependencies are passed in**, by constructor reference, so they are
  visible in the signatures and the construction order is checked by the
  compiler: `AddressSpace kernelSpace(pmm); Heap heap(kernelSpace);`
- **Hardware behind interfaces:** `InterruptController` (APIC, PIC),
  `TickSource` (APIC timer, PIT), `CharacterSink` (serial, VGA),
  `BlockDevice` and `FileSystem`.
- **`kernelMain` is the composition root.** It creates the objects in order,
  in static storage that is constructed explicitly (there is no heap yet and
  the boot stack is small), which also avoids static initialization order
  problems.
- **Globals only at C/assembly boundaries** (interrupt dispatch, the user
  program runtime behind `int 0x80` and ring 3 faults, liballoc hooks,
  `operator new`, the panic/printf sink): one explicitly bound pointer per
  boundary, set by the composition root. And one fact about the CPU: the
  **current task** (`Threading::currentTask()`, set by every switch), which
  sync objects and threads use to find the running thread and its scheduler.

`git blame` ignores the mechanical style commits listed in
`.git-blame-ignore-revs`; enable it once with
`git config blame.ignoreRevsFile .git-blame-ignore-revs`.
