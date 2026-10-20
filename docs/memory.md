# Memory management

Three layers, all in `kernel/mm/`:

```
kmalloc / kfree            heap.hpp      liballoc 1.1, 2 MB pages from the VMM
allocatePages / map        vmm.cpp       2 MB pages, recursive page tables
allocateBlocks / Frame     pmm.cpp       2 MB blocks, 4 KB frames for page tables
```

## Physical memory (`pmm.cpp`)

`Memory::PhysicalMemoryManager` keeps a bitmap with one bit per **2 MB
block**. The bitmap storage is passed to the constructor, so the capacity is
the caller's choice: `kernelMain` gives it a static buffer for 256 GB, the self
tests give small ones. The manager only does bookkeeping and never touches the
memory it describes.

- Everything starts in use. `addRegion(base, size)` frees whole, aligned
  blocks (beyond the capacity they are ignored); `reserveRegion(base, size)`
  marks every block it touches as used again.
- `Memory::populateFromMultiboot(pmm, mbInfo)` adds the `AVAILABLE` regions of
  the multiboot memory map (rounded inwards to 2 MB), then reserves
  `0 .. kernel_physical_end` (low memory, boot page tables, kernel image) and
  the multiboot info structure.
- `allocateBlocks(n)` returns `n` contiguous blocks (first fit) or `nullptr`;
  `freeBlocks(addr, n)` rejects unaligned addresses and double frees. Block 0
  is never handed out, its address would look like `nullptr`.
- `allocateFrame()` / `freeFrame()` hand out **4 KB frames** for page tables.
  A frame pool splits one 2 MB block into 512 frames (up to 16 pools); a pool
  that becomes empty gives its block back.
- `freeBlockCount()` is used by the self tests to detect leaks.
- `Memory::AddressSpace` gets the manager by reference in its constructor.

## Virtual memory (`vmm.cpp`)

`Memory::AddressSpace(pmm)` is the kernel's address space. Its constructor
takes over the active page tables (built by boot.S) and installs the recursive
mapping; pages and page directories come from the `pmm` it is given. The
recursive mapping only reaches the active tables, so an `AddressSpace` always
works on what CR3 points to; there is one today.

- Only **2 MB pages** are used (PD entries with the PS bit); there are no 4 KB
  page tables.
- Page tables are reached through a **recursive mapping** in `PML4[510]`, so
  the VMM never needs physical addresses to be mapped:

  | Table | Virtual address |
  |---|---|
  | PML4 | `[510][510][510][510]` |
  | PDPT for PML4 index `i` | `[510][510][510][i]` |
  | PD for indices `i, j` | `[510][510][i][j]` |

- PML4 and PDPT entries are written as raw 64-bit values;
  `PageDirectoryEntry64` describes 2 MB page entries only (its `frame_addr`
  starts at bit 21).
- `allocatePages(n)` finds `n` free pages between `kernel_virtual_end` and the
  top of the address space (at most 1024 pages, 2 GB), backs each with a PMM
  block and creates missing page directories from 4 KB frames.
- `freePages(addr, n)` unmaps, frees the blocks and gives a page directory back
  once it is empty (the boot page directories are kept).
- `mapPhysical(virt, phys)` / `unmapPhysical(virt)` map a caller provided
  physical 2 MB page (e.g. for MMIO) without touching the PMM.
- `virtToPhys(virt)` walks the tables (1 GB and 2 MB pages);
  it returns `nullptr` for unmapped addresses.
- Creating a new PDPT (a PML4 slot other than the kernel's) is not
  implemented and panics.

## Heap (`heap.hpp`, `liballoc/`)

`kmalloc`, `kfree`, `krealloc` and `kcalloc` come from liballoc 1.1 with a
page size of 2 MB; `liballoc_hooks.cpp` maps its page requests to
`AddressSpace::allocatePages` / `freePages`. liballoc calls these C hooks
without an object, so `kernelMain` binds the kernel address space once with
`Memory::setHeapAddressSpace`; a `kmalloc` before that panics. The liballoc
lock hooks are
no-ops: the scheduler is cooperative and interrupt handlers must not
allocate.
