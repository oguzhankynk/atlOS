# Memory management (`kernel/mm/`)

Three layers that turn "the RAM the firmware reports" into "memory a kernel
object can use": a physical memory manager, a virtual memory manager and a
heap. Everything else in the kernel allocates through them.

```
kmalloc / kfree / new / delete      heap.hpp, liballoc/   small objects, any size
AddressSpace::allocatePages         vmm.cpp               2 MB virtual pages
PhysicalMemoryManager               pmm.cpp               2 MB physical blocks, 4 KB frames
```

## What is it for?

A CPU in long mode never uses physical addresses directly: every access goes
through the **page tables**, which translate virtual addresses to physical
ones. So there are two separate questions:

1. **Which physical memory is free?** The firmware hands over a memory map
   (via multiboot2): "these ranges are RAM, these are reserved". Some of the
   RAM already holds the kernel. The *physical memory manager* (PMM) keeps
   track of what is free and hands out pieces of it.
2. **Where does that memory appear in the address space?** The *virtual memory
   manager* (VMM) writes page table entries so that a range of virtual
   addresses points at the physical memory the PMM gave out.

On top, kernel code wants `new Thread(...)` of 200 bytes, not "a 2 MB page".
The **heap** cuts pages into small allocations.

## How could it be designed?

### Physical memory

| Approach | How | Plus | Minus | Used by |
|---|---|---|---|---|
| **Bitmap** | one bit per page/block, scan for a 0 | tiny, simple, easy to reserve ranges | allocation scans; slow for huge RAM | many hobby OSes, early Linux |
| Free list | free pages linked through themselves | O(1) alloc/free | needs the memory mapped to write the links; contiguous runs are hard | xv6 (`kalloc`) |
| Stack of free frames | array of free addresses | O(1) | memory for the stack, no contiguity | some hobby OSes |
| Buddy allocator | power-of-two blocks, split and merge | fast, contiguous ranges, little fragmentation | more code | Linux (`page_alloc`) |

### Page size and table access

| Question | Options |
|---|---|
| Page size | 4 KB (fine grained, 4 table levels) · **2 MB** (3 levels, fewer tables) · 1 GB |
| How the kernel edits page tables | identity map them · **recursive mapping** (a PML4 entry points at the PML4 itself) · *physmap*: map all physical memory at a fixed offset (Linux) |

### Heap

| Approach | Plus | Minus |
|---|---|---|
| Bump allocator | 10 lines | never frees |
| Free list (first fit) | simple, frees | fragmentation, O(n) |
| **Third party allocator (liballoc)** | done, tested, frees and merges | someone else's code |
| Slab allocator | fast for fixed sizes | more code (Linux, Solaris) |

## How atlOS does it

**PMM: bitmap of 2 MB blocks** (`pmm.cpp`). The simplest structure that can
also *reserve* arbitrary ranges (the kernel image, the multiboot info). One
bit per 2 MB block; 16 KB of bitmap covers 256 GB. Allocation is first fit:
scan for `n` zero bits in a row. The bitmap storage is a constructor argument,
so the self tests build managers over a fake 128 MB "machine".

- `Memory::populateFromMultiboot` (`memory.cpp`) frees the `AVAILABLE`
  regions, then reserves `0 .. kernel_physical_end` (the firmware reports the
  kernel's memory as available too; forgetting this was one of the first
  bugs fixed in this repo).
- Block 0 is never handed out, its address would read as `nullptr`.
- Page tables are 4 KB, a whole 2 MB block for each would waste 2044 KB. So
  the PMM also hands out **4 KB frames**, cut from 2 MB blocks ("frame pools").

**VMM: 2 MB pages + recursive mapping** (`vmm.cpp`). With 2 MB pages there are
only three table levels (PML4 → PDPT → PD) and far fewer tables to manage.
To *edit* a table the kernel needs a virtual address for it; recursive mapping
gives one for free: `PML4[510]` points back at the PML4, so walking through
slot 510 lands on the tables themselves.

```
Virtual address of the PD for (i, j):   [510][510][i][j]
  PML4[510] -> PML4 used as a PDPT
  PDPT[510] -> PML4 used as a PD
  PD[i]     -> the PDPT of slot i, used as a page table
  PT[j]     -> the PD we want, seen as a 4 KB page
```

`Memory::AddressSpace` owns this: `allocatePages(n)` finds `n` free virtual
pages above the kernel, backs each with a PMM block and creates missing page
directories from 4 KB frames.

**Heap: liballoc 1.1.** Writing a good general allocator is a project of its
own; liballoc is small, public domain and needs only four hooks
(`liballoc_hooks.cpp`). Those hooks are C functions without an object, so
`kernelMain` binds the kernel `AddressSpace` once
(`Memory::setHeapAddressSpace`). A boundary global, see `docs/style.md`.

### Deliberately not done

- No 4 KB pages: fine for a kernel, not enough for user programs.
- Recursive mapping only reaches the **active** tables (CR3). A second address
  space (a process) needs another approach; Linux maps all physical memory
  (physmap) instead.
- No buddy allocator, no NUMA, no swapping, no copy-on-write.
- The PMM scan is linear; with 256 blocks (512 MB) nobody notices.

## Flow: `new Thread(...)` down to a page table entry

```mermaid
sequenceDiagram
    participant C as Caller
    participant H as liballoc (kmalloc)
    participant V as AddressSpace
    participant P as PhysicalMemoryManager
    participant T as Page tables
    C->>H: kmalloc(200)
    alt a page with free space exists
        H-->>C: pointer inside it
    else needs more memory
        H->>V: liballoc_alloc(1 page) -> allocatePages(1)
        V->>V: find a free 2 MB virtual page
        V->>P: allocateFrame() (only if a new PD is needed)
        V->>P: allocateBlocks(1)
        P-->>V: physical address of a 2 MB block
        V->>T: write the PD entry (present, writable, 2 MB)
        V-->>H: virtual address
        H-->>C: pointer inside the new page
    end
```

## Boot: from the memory map to a working heap

```mermaid
flowchart TD
    A[GRUB: multiboot2 memory map] --> B[PhysicalMemoryManager: everything used]
    B --> C[addRegion for each AVAILABLE range]
    C --> D[reserveRegion: kernel image, multiboot info]
    D --> E[AddressSpace: install recursive mapping in PML4]
    E --> F[setHeapAddressSpace: kmalloc can now get pages]
```

## Tests

`kernel/tests/selftest.cpp`:

- *PMM blocks / frames (isolated)*: a 64 block manager: regions, reservations,
  exhaustion, double frees, first fit, frame pools and their limit.
- *PMM from a multiboot memory map*: a hand built multiboot2 structure.
- *VMM*: allocate, write, free, translate, map the same physical page twice.
- *Heap*: sizes from 1 byte to 3 MB, `krealloc`, `kcalloc`.

## References

- OSDev Wiki: [Paging](https://wiki.osdev.org/Paging),
  [Page Frame Allocation](https://wiki.osdev.org/Page_Frame_Allocation),
  [Detecting Memory (x86)](https://wiki.osdev.org/Detecting_Memory_(x86)),
  [Higher Half Kernel](https://wiki.osdev.org/Higher_Half_Kernel)
- Intel SDM Vol. 3A, chapter "Paging" (4-level paging, PS bit for 2 MB pages)
- *Operating Systems: Three Easy Pieces*: "Paging: Introduction", "Free-Space Management"
- xv6 book (MIT 6.1810): chapter "Page tables"
- [liballoc](https://github.com/blanham/liballoc)
