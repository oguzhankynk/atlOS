# Memory management

Three layers, all in `kernel/mm/`:

```
kmalloc / kfree            heap.hpp      liballoc 1.1, 2 MB pages from the VMM
allocatePages / map        vmm.cpp       2 MB pages, page tables through the physmap
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

`Memory::AddressSpace(pmm)` is the kernel's address space: it takes over the
page tables built by boot.S and adds the **physmap**, all physical memory (up
to 64 GB) at `0xFFFF800000000000 + physical` (`PML4[256]`). Page tables hold
physical addresses; the VMM reads and writes them through the physmap, so an
`AddressSpace` works on its tables whether they are in CR3 or not.
`AddressSpace(pmm, kernel)` creates another one (for a user program) that
shares the kernel's `PML4[0]` and `PML4[256..511]`.

- Only **2 MB pages** are used (PD entries with the PS bit), also for user
  programs.
- `allocatePages(n)` finds `n` free pages between `kernel_virtual_end` and the
  top of the address space, backs each with a PMM block and creates missing
  page directories from 4 KB frames. `freePages` gives the blocks and empty
  page directories back (not the ones in the kernel image).
- `mapPhysical(virt, phys)` / `unmapPhysical(virt)` map a caller provided
  physical 2 MB page without touching the PMM.
- `mapDevice(phys)` maps the 2 MB page around a device's registers at a free
  kernel address, **uncached** (the PCD and PWT bits): the physmap is cached,
  and a device must see every read and write. The local APIC and the I/O
  APICs are reached this way.
- `allocateAt(virt, n, flags)` / `freeAt` map zeroed pages in the user range
  (`userBase` = `0x0000008000000000` .. `userEnd`), with `Writable` and `User`.
- Bit 9 of an entry (ignored by the CPU) marks pages whose block the address
  space owns; only those are freed on unmap and destruction.
- `translate(virt, phys)` and `virtToPhys(virt)` walk the tables;
  `forEachMapping` lists every mapped page. `activate()` loads CR3.
- The kernel half has a fixed set of PDPTs (they are shared by every address
  space); a mapping that needs a new one there panics.

## Heap (`heap.hpp`, `liballoc/`)

`kmalloc`, `kfree`, `krealloc` and `kcalloc` come from liballoc 1.1 with a
page size of 2 MB; `liballoc_hooks.cpp` maps its page requests to
`AddressSpace::allocatePages` / `freePages`. liballoc calls these C hooks
without an object, so `kernelMain` binds the kernel address space once with
`Memory::setHeapAddressSpace`; a `kmalloc` before that panics. The liballoc
lock hooks are
no-ops: the scheduler is cooperative and interrupt handlers must not
allocate.
