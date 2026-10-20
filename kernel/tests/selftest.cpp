/**
 * @file selftest.cpp
 * @author Oguzhan KAYNAK
 * @brief In-kernel self tests for the memory managers, heap, interrupts and scheduler.
 *
 * Every check prints only on failure; each group ends with a leak check
 * against the PMM free block count where it applies.
 */

#include <tests/selftest.hpp>

#include <stdio.h>
#include <string.h>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/cpu/backtrace.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/fault.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <core/console.hpp>
#include <cxx/uninitialized.hpp>
#include <drivers/character_sink.hpp>
#include <drivers/disk/ata.hpp>
#include <drivers/disk/memory_block_device.hpp>
#include <drivers/disk/partition.hpp>
#include <drivers/interrupt_controller.hpp>
#include <drivers/keyboard.hpp>
#include <drivers/pit.hpp>
#include <drivers/scancode.hpp>
#include <drivers/serial.hpp>
#include <drivers/tick_source.hpp>
#include <drivers/vga_console.hpp>
#include <fs/fat16.hpp>
#include <fs/vfs.hpp>
#include <mm/constants.hpp>
#include <mm/heap.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>
#include <sync/atomic.hpp>
#include <sync/condition_variable.hpp>
#include <sync/interrupt_guard.hpp>
#include <sync/mutex.hpp>
#include <sync/semaphore.hpp>
#include <sync/spinlock.hpp>
#include <sync/wait_queue.hpp>

namespace SelfTest {
using namespace Memory;

static int s_passed {};
static int s_failed {};
static const char* s_group {};

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check(bool ok, const char* expr, int line)
{
    if (ok) {
        s_passed++;
        return;
    }

    s_failed++;
    printf("[SELFTEST] FAIL %s (line %d): %s\n", s_group, line, expr);
}

static void group(const char* name)
{
    s_group = name;
    printf("[SELFTEST] %s\n", name);
}

static bool aboveKernel(const void* phys)
{
    return (uintptr_t)phys >= (uintptr_t)&kernel_physical_end;
}

// ------------------------------------------------------------
// PMM
//
// The manager never touches the memory it describes, so these tests build
// their own small instances instead of draining the kernel's.
// ------------------------------------------------------------

static constexpr uint64_t blockSize = PhysicalMemoryManager::blockSize;

static void* blockAddress(uint64_t block)
{
    return reinterpret_cast<void*>(block * blockSize);
}

static bool isBlock(const void* address, uint64_t block)
{
    return address == blockAddress(block);
}

static void testPmmBlocks()
{
    group("PMM blocks (isolated)");

    static uint8_t s_bitmap[8]; // 64 blocks, 128 MB
    PhysicalMemoryManager pmm(s_bitmap, sizeof(s_bitmap));

    CHECK(pmm.capacity() == 64);
    CHECK(pmm.freeBlockCount() == 0);
    CHECK(pmm.allocateBlocks(1) == nullptr);

    // Blocks 0..31; block 0 is never handed out
    pmm.addRegion(blockAddress(0), 32 * blockSize);
    CHECK(pmm.freeBlockCount() == 31);

    // An unaligned region is rejected
    pmm.addRegion((uint8_t*)blockAddress(40) + 4096, 4 * blockSize);
    CHECK(pmm.freeBlockCount() == 31);

    // Reserving covers every block it touches
    pmm.reserveRegion(blockAddress(5), 3 * blockSize); // 5..7
    pmm.reserveRegion((uint8_t*)blockAddress(8) + 100, 10); // 8
    CHECK(pmm.freeBlockCount() == 27);

    // Every free block can be allocated, then nullptr
    static void* s_blocks[64];
    uint64_t n = 0;
    bool valid = true;
    while (n < 64 && (s_blocks[n] = pmm.allocateBlocks(1)) != nullptr) {
        uint64_t address = (uint64_t)s_blocks[n];
        uint64_t block = address / blockSize;
        valid &= address % blockSize == 0 && block >= 1 && block < 32 && (block < 5 || block > 8);
        n++;
    }
    CHECK(n == 27);
    CHECK(valid);
    CHECK(pmm.freeBlockCount() == 0);
    CHECK(pmm.allocateBlocks(1) == nullptr);

    bool unique = true;
    for (uint64_t i = 0; i < n; i++)
        for (uint64_t j = i + 1; j < n; j++)
            unique &= s_blocks[i] != s_blocks[j];
    CHECK(unique);

    for (uint64_t i = 0; i < n; i++)
        pmm.freeBlocks(s_blocks[i], 1);
    CHECK(pmm.freeBlockCount() == 27);

    // Bad frees are rejected (each logs an error) and do not change the count
    printf("[SELFTEST] expecting three freeBlocks errors:\n");
    pmm.freeBlocks(s_blocks[0], 1); // double free
    pmm.freeBlocks((uint8_t*)blockAddress(2) + 4096, 1); // unaligned
    pmm.freeBlocks(blockAddress(40), 1); // never added
    CHECK(pmm.freeBlockCount() == 27);

    // Contiguous allocations are first fit and skip reserved blocks
    void* first = pmm.allocateBlocks(4);
    void* second = pmm.allocateBlocks(4);
    CHECK(isBlock(first, 1)); // 1..4
    CHECK(isBlock(second, 9)); // 9..12, after the reserved 5..8
    pmm.freeBlocks(first, 4);
    pmm.freeBlocks(second, 4);

    // Enough free blocks but no run that long
    CHECK(pmm.allocateBlocks(24) == nullptr);
    void* longest = pmm.allocateBlocks(23); // 9..31
    CHECK(isBlock(longest, 9));
    pmm.freeBlocks(longest, 23);
    CHECK(pmm.freeBlockCount() == 27);

    // Memory beyond the capacity is ignored
    pmm.addRegion(blockAddress(60), 10 * blockSize); // only 60..63 exist
    CHECK(pmm.freeBlockCount() == 31);
}

static void testPmmFrames()
{
    group("PMM frames (isolated)");

    static uint8_t s_bitmap[8];
    PhysicalMemoryManager pmm(s_bitmap, sizeof(s_bitmap));
    pmm.addRegion(blockAddress(0), 32 * blockSize); // blocks 1..31
    uint64_t freeBefore = pmm.freeBlockCount();

    // More than two pools of 512 frames
    constexpr int n = 1100;
    static void* s_frames[n];

    bool allocated = true, aligned = true, inRange = true;
    for (int i = 0; i < n; i++) {
        s_frames[i] = pmm.allocateFrame();
        if (s_frames[i] == nullptr) {
            allocated = false;
            break;
        }
        aligned &= ((uintptr_t)s_frames[i] % frameSize4Kb) == 0;
        inRange &= s_frames[i] >= blockAddress(1) && s_frames[i] < blockAddress(32);
    }
    CHECK(allocated);
    if (!allocated)
        return;
    CHECK(aligned);
    CHECK(inRange);
    CHECK(pmm.freeBlockCount() == freeBefore - 3);

    bool unique = true;
    for (int i = 0; i < n && unique; i++)
        for (int j = i + 1; j < n; j++)
            if (s_frames[i] == s_frames[j]) {
                unique = false;
                break;
            }
    CHECK(unique);

    // A freed frame is handed out again
    pmm.freeFrame(s_frames[5]);
    void* again = pmm.allocateFrame();
    CHECK(again == s_frames[5]);
    s_frames[5] = again;

    // A double free is rejected (logs an error), so the frame is not handed out twice
    printf("[SELFTEST] expecting a freeFrame error:\n");
    pmm.freeFrame(s_frames[6]);
    pmm.freeFrame(s_frames[6]);
    void* a = pmm.allocateFrame();
    void* b = pmm.allocateFrame();
    CHECK(a != b);
    s_frames[6] = a;
    pmm.freeFrame(b);

    for (int i = 0; i < n; i++)
        pmm.freeFrame(s_frames[i]);

    // Empty pools give their blocks back
    CHECK(pmm.freeBlockCount() == freeBefore);

    // The pools are bounded: maxFramePools blocks of framesPerBlock frames
    constexpr uint64_t limit = PhysicalMemoryManager::maxFramePools * PhysicalMemoryManager::framesPerBlock;
    static void* s_all[limit];
    uint64_t count = 0;
    void* frame;
    while (count < limit + 1 && (frame = pmm.allocateFrame()) != nullptr)
        s_all[count++] = frame;
    CHECK(count == limit);
    CHECK(pmm.freeBlockCount() == freeBefore - PhysicalMemoryManager::maxFramePools);

    for (uint64_t i = 0; i < count; i++)
        pmm.freeFrame(s_all[i]);
    CHECK(pmm.freeBlockCount() == freeBefore);
}

static void testPmmMultiboot()
{
    group("PMM from a multiboot memory map");

    // multiboot2 info: header, one memory map tag with three entries, end tag
    static constexpr uint32_t entries = 3;
    static constexpr uint32_t mmapSize = sizeof(multiboot_tag_mmap) + entries * sizeof(multiboot_mmap_entry);
    static constexpr uint32_t totalSize = 8 + mmapSize + sizeof(multiboot_tag);
    alignas(8) static uint8_t s_info[totalSize];

    memset(s_info, 0, sizeof(s_info));
    uint32_t header[2] = { totalSize, 0 };
    memcpy(s_info, header, sizeof(header));

    multiboot_tag_mmap mmap {};
    mmap.type = MULTIBOOT_TAG_TYPE_MMAP;
    mmap.size = mmapSize;
    mmap.entry_size = sizeof(multiboot_mmap_entry);
    memcpy(s_info + 8, &mmap, sizeof(mmap));

    // A typical PC: low memory, a hole below 1 MB, then 128 MB from 1 MB on
    multiboot_mmap_entry map[entries] = {
        { 0x0, 0x9FC00, MULTIBOOT_MEMORY_AVAILABLE, 0 },
        { 0x9FC00, 0x400, MULTIBOOT_MEMORY_RESERVED, 0 },
        { 0x100000, 0x7F00000, MULTIBOOT_MEMORY_AVAILABLE, 0 },
    };
    memcpy(s_info + 8 + sizeof(mmap), map, sizeof(map));

    multiboot_tag end { MULTIBOOT_TAG_TYPE_END, sizeof(multiboot_tag) };
    memcpy(s_info + 8 + mmapSize, &end, sizeof(end));

    static uint8_t s_bitmap[8]; // 64 blocks
    PhysicalMemoryManager pmm(s_bitmap, sizeof(s_bitmap));
    populateFromMultiboot(pmm, (uintptr_t)s_info);

    // The second AVAILABLE entry rounds inwards to blocks 1..63; the kernel
    // (below kernel_physical_end) is reserved. The first entry is smaller than
    // a block, and this fake info lives in the higher half, beyond the capacity.
    uint64_t kernelBlocks = (uint64_t)&kernel_physical_end / blockSize;
    CHECK(pmm.freeBlockCount() == 63 - (kernelBlocks - 1));

    void* block = pmm.allocateBlocks(1);
    CHECK(isBlock(block, kernelBlocks));
}

static void testPmmKernel(PhysicalMemoryManager& pmm)
{
    group("PMM (kernel instance)");

    uint64_t freeBefore = pmm.freeBlockCount();
    CHECK(freeBefore > 0);

    void* block = pmm.allocateBlocks(1);
    CHECK(block != nullptr && aboveKernel(block));
    CHECK((uintptr_t)block % blockSize == 0);
    pmm.freeBlocks(block, 1);

    void* frame = pmm.allocateFrame();
    CHECK(frame != nullptr && aboveKernel(frame));
    pmm.freeFrame(frame);

    CHECK(pmm.freeBlockCount() == freeBefore);
}

// ------------------------------------------------------------
// VMM
// ------------------------------------------------------------

static void testVmm(PhysicalMemoryManager& pmm, AddressSpace& space)
{
    group("VMM");
    uint64_t freeBefore = pmm.freeBlockCount();

    // Allocate, touch the first and last word of every page, free
    constexpr uint64_t n = 8;
    constexpr uint64_t words = pageSize2Mb / sizeof(uint64_t);
    uint64_t* base = (uint64_t*)space.allocatePages(n);
    CHECK((uintptr_t)base >= (uintptr_t)&kernel_virtual_end);
    CHECK(pmm.freeBlockCount() == freeBefore - n);

    for (uint64_t p = 0; p < n; p++) {
        base[p * words] = 0xA5A5000000000000ULL | p;
        base[p * words + words - 1] = 0x5A5A000000000000ULL | p;
    }
    bool intact = true;
    for (uint64_t p = 0; p < n; p++) {
        intact &= base[p * words] == (0xA5A5000000000000ULL | p);
        intact &= base[p * words + words - 1] == (0x5A5A000000000000ULL | p);
    }
    CHECK(intact);

    space.freePages(base, n);
    CHECK(pmm.freeBlockCount() == freeBefore);

    // The freed range is found again
    void* again = space.allocatePages(n);
    CHECK(again == base);
    space.freePages(again, n);

    // Translation: kernel image, low identity map, unmapped
    uintptr_t kernelVirt = (uintptr_t)&run;
    CHECK((uintptr_t)space.virtToPhys((void*)kernelVirt) == kernelVirt - KERNEL_VIRTUAL_LOAD_OFFSET);
    CHECK(space.virtToPhys((void*)0x123456) == (void*)0x123456);
    CHECK(space.virtToPhys((void*)0x0000700000000000ULL) == nullptr);

    // Map a second view of a heap page. The alias lives in PDPT[511],
    // which has no PD yet, so this also creates and frees a PD.
    uint64_t* page = (uint64_t*)space.allocatePages(1);
    uintptr_t phys = (uintptr_t)space.virtToPhys(page);
    CHECK(phys != 0 && aboveKernel((void*)phys));
    CHECK((uintptr_t)space.virtToPhys(page + 3) == phys + 3 * sizeof(uint64_t));

    uint64_t* alias = (uint64_t*)0xFFFFFFFFF0000000ULL;
    CHECK(space.virtToPhys(alias) == nullptr);
    space.mapPhysical(alias, (void*)phys);
    CHECK((uintptr_t)space.virtToPhys(alias) == phys);

    page[3] = 0x1122334455667788ULL;
    CHECK(alias[3] == 0x1122334455667788ULL);

    space.unmapPhysical(alias);
    CHECK(space.virtToPhys(alias) == nullptr);
    CHECK(page[3] == 0x1122334455667788ULL);

    space.freePages(page, 1);

    // Includes the PD frame pool created for the alias
    CHECK(pmm.freeBlockCount() == freeBefore);
}

// ------------------------------------------------------------
// Heap
// ------------------------------------------------------------

static void testHeap()
{
    group("Heap");

    // The last one spans more than one 2 MB page
    constexpr int n = 7;
    const size_t sizes[n] = { 1, 16, 100, 1000, 4096, 100000, size_t(3) * 1024 * 1024 };
    uint8_t* ptrs[n];

    bool allocated = true;
    for (int i = 0; i < n; i++) {
        ptrs[i] = (uint8_t*)kmalloc(sizes[i]);
        allocated &= ptrs[i] != nullptr;
    }
    CHECK(allocated);
    if (!allocated)
        return;

    for (int i = 0; i < n; i++)
        memset(ptrs[i], i + 1, sizes[i]);

    // Writing one block must not have touched another
    bool intact = true;
    for (int i = 0; i < n; i++)
        for (size_t b = 0; b < sizes[i]; b++)
            if (ptrs[i][b] != i + 1) {
                intact = false;
                break;
            }
    CHECK(intact);

    bool disjoint = true;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (ptrs[i] < ptrs[j] + sizes[j] && ptrs[j] < ptrs[i] + sizes[i])
                disjoint = false;
    CHECK(disjoint);

    for (int i = 0; i < n; i++)
        kfree(ptrs[i]);

    // krealloc keeps the old contents
    uint8_t* r = (uint8_t*)kmalloc(32);
    for (int i = 0; i < 32; i++)
        r[i] = i;
    r = (uint8_t*)krealloc(r, 5000);
    CHECK(r != nullptr);
    bool kept = r != nullptr;
    for (int i = 0; i < 32 && kept; i++)
        kept = r[i] == i;
    CHECK(kept);
    kfree(r);

    // kcalloc returns zeroed memory
    uint8_t* z = (uint8_t*)kcalloc(100, 10);
    CHECK(z != nullptr);
    bool zero = z != nullptr;
    for (int i = 0; i < 1000 && zero; i++)
        zero = z[i] == 0;
    CHECK(zero);
    kfree(z);
}

// ------------------------------------------------------------
// Interrupts
// ------------------------------------------------------------

static void testInterrupts(Threading::Scheduler& scheduler)
{
    group("Interrupts");

    // The timer is running
    uint64_t t0 = scheduler.ticks();
    for (int i = 0; i < 5; i++)
        asm volatile("sti; hlt");
    CHECK(scheduler.ticks() > t0);

    // An interrupt must not change the registers of the interrupted code
    uint64_t rax, r12;
    asm volatile("movq $0x1234567890ABCDEF, %%rax\n\t"
                 "movq $0x0FEDCBA987654321, %%r12\n\t"
                 "sti\n\t"
                 "hlt\n\t"
                 "movq %%rax, %0\n\t"
                 "movq %%r12, %1"
                 : "=r"(rax), "=r"(r12)
                 :
                 : "rax", "r12", "memory");
    CHECK(rax == 0x1234567890ABCDEFULL);
    CHECK(r12 == 0x0FEDCBA987654321ULL);
}

// ------------------------------------------------------------
// Uninitialized<T>
// ------------------------------------------------------------

struct Counted {
    explicit Counted(int value)
        : value(value)
    {
        s_constructions++;
    }
    int value;
    static int s_constructions;
};
int Counted::s_constructions = 0;

static void testUninitialized()
{
    group("Uninitialized");

    static Cxx::Uninitialized<Counted> s_storage;
    CHECK(!s_storage.isConstructed());
    CHECK(Counted::s_constructions == 0);

    Counted& object = s_storage.construct(42);
    CHECK(s_storage.isConstructed());
    CHECK(Counted::s_constructions == 1);
    CHECK(object.value == 42);
    CHECK(&s_storage.get() == &object);
    CHECK(s_storage->value == 42);
}

// ------------------------------------------------------------
// Drivers through their interfaces
// ------------------------------------------------------------

// Records what a driver asks of its interrupt controller
class FakeInterruptController final : public Drivers::InterruptController {
public:
    bool attach(Drivers::Irq irq, InterruptHandlerFunction handler, void* arg) override
    {
        attachCount++;
        attachedIrq = irq;
        attachedHandler = handler;
        attachedArg = arg;
        return true;
    }
    void mask(Drivers::Irq) override { }
    void unmask(Drivers::Irq irq) override
    {
        unmaskCount++;
        unmaskedIrq = irq;
    }
    void endOfInterrupt(uint32_t) override { }

    int attachCount = 0;
    int unmaskCount = 0;
    Drivers::Irq attachedIrq {};
    Drivers::Irq unmaskedIrq {};
    InterruptHandlerFunction attachedHandler {};
    void* attachedArg {};
};

static void fakeTick(void*) { }

static void testPitWithFakeController(Threading::Scheduler& scheduler)
{
    group("PIT (fake interrupt controller)");

    FakeInterruptController controller;
    Drivers::PIT pit(controller);

    // Out of range periods are refused before anything is touched
    CHECK(!pit.start(100000000, fakeTick, nullptr)); // 100 ms
    CHECK(!pit.start(100, fakeTick, nullptr)); // 100 ns
    CHECK(controller.attachCount == 0);

    // A valid period attaches and unmasks the timer IRQ. This reprograms the
    // real channel 0 with the scheduler's own 1 ms, so the tick is unchanged;
    // the real IRQ0 handler stays the scheduler's because the fake does not
    // touch the IDT handlers.
    int marker;
    CHECK(pit.start(1000000, fakeTick, &marker));
    CHECK(controller.attachCount == 1);
    CHECK(controller.attachedIrq == Drivers::Irq::Timer);
    CHECK(controller.attachedHandler == fakeTick);
    CHECK(controller.attachedArg == &marker);
    CHECK(controller.unmaskCount == 1);
    CHECK(controller.unmaskedIrq == Drivers::Irq::Timer);

    // The scheduler tick keeps running
    uint64_t t0 = scheduler.ticks();
    scheduler.sleep(5);
    CHECK(scheduler.ticks() > t0);
}

// ------------------------------------------------------------
// Console
// ------------------------------------------------------------

// Collects what is written to it
class FakeSink final : public Drivers::CharacterSink {
public:
    void write(const char* data, size_t size) override
    {
        for (size_t i = 0; i < size && m_length + 1 < sizeof(m_text); i++)
            m_text[m_length++] = data[i];
        m_text[m_length] = '\0';
    }

    const char* text() const { return m_text; }

private:
    char m_text[64] {};
    size_t m_length {};
};

static void testConsole()
{
    group("Console (fake sink)");

    FakeSink sink;
    Drivers::CharacterSink* previous = Core::setConsole(sink);
    printf("x=%d\n", 42);
    Core::setConsole(*previous);

    CHECK(previous != nullptr);
    CHECK(strcmp(sink.text(), "x=42\n") == 0);
}

static char screenChar(const volatile uint16_t* screen, size_t x, size_t y)
{
    return static_cast<char>(screen[y * Drivers::VGATextConsole::width + x] & 0xFF);
}

static void writeText(Drivers::CharacterSink& sink, const char* text)
{
    sink.write(text, strlen(text));
}

static void testVgaConsole()
{
    group("VGA text console (RAM buffer)");

    using Drivers::VGATextConsole;
    constexpr size_t width = VGATextConsole::width;
    constexpr size_t height = VGATextConsole::height;
    static volatile uint16_t s_screen[width * height];

    // Cleared to light grey spaces
    {
        VGATextConsole console(s_screen);
        bool cleared = true;
        for (size_t i = 0; i < width * height; i++)
            cleared &= s_screen[i] == 0x0720;
        CHECK(cleared);

        writeText(console, "AB\nC\tD");
        CHECK(screenChar(s_screen, 0, 0) == 'A');
        CHECK(screenChar(s_screen, 1, 0) == 'B');
        CHECK(screenChar(s_screen, 0, 1) == 'C');
        CHECK(screenChar(s_screen, 1, 1) == ' '); // tab: four spaces
        CHECK(screenChar(s_screen, 5, 1) == 'D');

        console.setColor(0x1F);
        writeText(console, "E");
        CHECK(s_screen[1 * width + 6] == ((0x1F << 8) | 'E'));
    }

    // A full line wraps to the next one
    {
        VGATextConsole console(s_screen);
        for (size_t i = 0; i < width + 1; i++)
            writeText(console, "x");
        CHECK(screenChar(s_screen, width - 1, 0) == 'x');
        CHECK(screenChar(s_screen, 0, 1) == 'x');
        CHECK(screenChar(s_screen, 1, 1) == ' ');
    }

    // 26 lines: the screen scrolls twice and shows "c".."z", the last row empty
    {
        VGATextConsole console(s_screen);
        for (char letter = 'a'; letter <= 'z'; letter++) {
            char line[2] = { letter, '\n' };
            console.write(line, 2);
        }
        CHECK(screenChar(s_screen, 0, 0) == 'c');
        CHECK(screenChar(s_screen, 0, height - 2) == 'z');
        CHECK(screenChar(s_screen, 0, height - 1) == ' ');
    }

    // Backspace moves back a cell, across the line start, not past the top
    {
        VGATextConsole console(s_screen);
        writeText(console, "ab\bc");
        CHECK(screenChar(s_screen, 0, 0) == 'a' && screenChar(s_screen, 1, 0) == 'c');
        writeText(console, "\n\bz");
        CHECK(screenChar(s_screen, width - 1, 0) == 'z');
        VGATextConsole top(s_screen);
        writeText(top, "\bq");
        CHECK(screenChar(s_screen, 0, 0) == 'q');
    }
}

static void testSerialPortMissing()
{
    group("Serial port (missing COM2)");

    // Writing to a port that failed its loopback test must return instead of
    // waiting for the transmitter. Only testable on a machine without COM2.
    Drivers::SerialPort com2(Drivers::SerialPort::com2);
    if (com2.isPresent()) {
        printf("[SELFTEST] skipped: this machine has a COM2\n");
        return;
    }
    writeText(com2, "dropped");
    CHECK(true); // reached
}

// ------------------------------------------------------------
// Faults: decoding, TSS, backtrace
// ------------------------------------------------------------

static void testFaultDecoding()
{
    group("Fault decoding");
    using namespace Fault;

    PageFaultError pf = decodePageFault(0);
    CHECK(!pf.present && !pf.write && !pf.user && !pf.reservedBit && !pf.instructionFetch);
    pf = decodePageFault(0x7); // a user write to a present page
    CHECK(pf.present && pf.write && pf.user && !pf.reservedBit && !pf.instructionFetch);
    pf = decodePageFault(0x8);
    CHECK(!pf.present && pf.reservedBit);
    pf = decodePageFault(0x10);
    CHECK(!pf.write && pf.instructionFetch);

    SelectorError se = decodeSelectorError(0x28); // the TSS descriptor
    CHECK(!se.external && se.table == DescriptorTable::GDT && se.index == 5);
    se = decodeSelectorError((14 << 3) | 0x2 | 0x1); // IDT vector 14, external event
    CHECK(se.external && se.table == DescriptorTable::IDT && se.index == 14);
    se = decodeSelectorError((3 << 3) | 0x4);
    CHECK(se.table == DescriptorTable::LDT && se.index == 3);
    se = decodeSelectorError((3 << 3) | 0x6); // IDT wins, TI is ignored then
    CHECK(se.table == DescriptorTable::IDT);
    se = decodeSelectorError(0xFFF8);
    CHECK(se.index == 0x1FFF);

    CHECK(strcmp(exceptionName(0), "Divide error") == 0);
    CHECK(strcmp(exceptionName(8), "Double fault") == 0);
    CHECK(strcmp(exceptionName(13), "General protection fault") == 0);
    CHECK(strcmp(exceptionName(14), "Page fault") == 0);
    CHECK(exceptionName(32) == nullptr);

    // The vector enum once had an extra entry, shifting 10..21 by one
    CHECK((uint32_t)IDT::ExternalInterrupts::InvalidTSS == 10);
    CHECK((uint32_t)IDT::ExternalInterrupts::PageFault == 14);
    CHECK((uint32_t)IDT::ExternalInterrupts::ControlProtectionException == 21);
}

static void testTaskStateSegment()
{
    group("TSS and double fault stack");

    uint16_t taskRegister;
    __asm__ volatile("str %0"
                     : "=r"(taskRegister));
    CHECK(taskRegister == (uint16_t)SegmentSelect::TSS);

    const GDT::TaskStateSegment& tss = GDT::taskStateSegment();
    uint64_t top = tss.ist[GDT::doubleFaultStack - 1];
    CHECK(top != 0 && top % 16 == 0);
    CHECK(tss.ioMapBase == sizeof(GDT::TaskStateSegment));

    // Only the double fault switches stacks
    CHECK(IDT::entry(8).ist == GDT::doubleFaultStack);
    CHECK(IDT::entry(14).ist == 0);
    CHECK(IDT::entry(32).ist == 0);

    // The descriptor's busy bit: ltr marks the TSS as in use (type 9 -> 11)
    const GDT::GDTR& gdtr = GDT::getGDTR();
    const GDT::GDTEntry* gdt = reinterpret_cast<const GDT::GDTEntry*>(gdtr.base);
    const GDT::GDTEntry& descriptor = gdt[(uint16_t)SegmentSelect::TSS / sizeof(GDT::GDTEntry)];
    CHECK(descriptor.access == 0x8B);
    CHECK(gdtr.limit == 7 * sizeof(GDT::GDTEntry) - 1);
}

static bool inKernelText(uint64_t address)
{
    return address >= (uint64_t)&_text_start && address < (uint64_t)&_text_end;
}

__attribute__((noinline)) static int backtraceHere(uint64_t* addresses, int max)
{
    return Backtrace::collect((uint64_t)__builtin_frame_address(0), addresses, max);
}

static void testBacktrace()
{
    group("Backtrace");

    // backtraceHere <- testBacktrace <- run <- kernelMain <- boot.S
    uint64_t addresses[Backtrace::maxFrames] {};
    int count = backtraceHere(addresses, Backtrace::maxFrames);
    CHECK(count >= 4);
    CHECK(count < Backtrace::maxFrames); // the chain ends at boot.S's rbp = 0
    CHECK(inKernelText(addresses[0]));
    CHECK(inKernelText(addresses[1]));

    // The limit is respected
    uint64_t two[3] {};
    CHECK(backtraceHere(two, 2) == 2);
    CHECK(two[2] == 0);

    // Broken chains end the walk
    CHECK(Backtrace::collect(0, addresses, Backtrace::maxFrames) == 0);
    CHECK(Backtrace::collect(0x1003, addresses, Backtrace::maxFrames) == 0);

    // A frame whose saved rbp points down the stack: one address, then stop
    uint64_t loop[2] = { 0, 0x1234 };
    loop[0] = (uint64_t)&loop[0];
    CHECK(Backtrace::collect((uint64_t)&loop[0], addresses, Backtrace::maxFrames) == 1);
    CHECK(addresses[0] == 0x1234);
}

// ------------------------------------------------------------
// CPU
// ------------------------------------------------------------

static void testCpu()
{
    group("CPU");

    static CPUInfo s_info;
    initializeCpuInfo(&s_info);
    printCPUInfo(&s_info); // only to the serial log

    // Every x86_64 CPU has leaf 1, SSE2 and FXSR
    CHECK(s_info.maksimumIndex >= 1);
    CHECK(s_info.cpuFeaturesInfo.sse2 == 1);
    CHECK(s_info.cpuFeaturesInfo.fxsr == 1);

    // Vendor string such as "GenuineIntel"
    bool printable = strlen(s_info.cpu) == 12;
    for (size_t i = 0; i < 12 && printable; i++)
        printable = s_info.cpu[i] >= 0x20 && s_info.cpu[i] < 0x7f;
    CHECK(printable);

    CHECK(s_info.cacheList.cacheCount <= sizeof(s_info.cacheList.cacheInfo) / sizeof(s_info.cacheList.cacheInfo[0]));
}

// ------------------------------------------------------------
// Threads and scheduler
//
// Most tests run on their own Scheduler driven by a FakeTickSource, so time
// only moves when the test says so. The calling flow is task 0 of both that
// scheduler and the kernel's; while a test scheduler runs, the kernel's simply
// sees its task 0 running.
// ------------------------------------------------------------

using Threading::Scheduler;
using Threading::Thread;
using Threading::ThreadState;

// A tick source the test advances by hand
class FakeTickSource final : public Drivers::TickSource {
public:
    bool start(uint32_t periodNs, TickHandler handler, void* arg) override
    {
        m_handler = handler;
        m_arg = arg;
        return periodNs == Scheduler::tickPeriodNs;
    }

    void advance(uint64_t ticks)
    {
        for (uint64_t i = 0; i < ticks; i++)
            m_handler(m_arg);
    }

private:
    TickHandler m_handler {};
    void* m_arg {};
};

static constexpr uint64_t rounds = 100;

static volatile uint64_t s_countA {};
static volatile uint64_t s_countB {};
static volatile bool s_frameAligned {};

static void counterA(Scheduler& scheduler)
{
    // After "push rbp" the frame must be 16 byte aligned if the thread was
    // started with an ABI conformant stack
    s_frameAligned = ((uintptr_t)__builtin_frame_address(0) % 16) == 0;

    for (uint64_t i = 0; i < rounds; i++) {
        s_countA = s_countA + 1;
        scheduler.yield();
    }
}

static void counterB(Scheduler& scheduler)
{
    for (uint64_t i = 0; i < rounds; i++) {
        s_countB = s_countB + 1;
        scheduler.yield();
    }
}

/**
 * @brief Yield until the thread has exited, for at most maxMs ticks (and a
 * bounded number of rounds, in case the ticks do not move)
 */
static bool waitForExit(Scheduler& scheduler, Thread& thread, uint64_t maxMs)
{
    uint64_t deadline = scheduler.ticks() + maxMs;

    for (uint64_t i = 0; i < 10000000; i++) {
        if (thread.getTCB().state == ThreadState::Terminated || scheduler.ticks() >= deadline)
            break;
        scheduler.yield();
    }

    return thread.getTCB().state == ThreadState::Terminated;
}

static void testThreadStack(Scheduler& scheduler, AddressSpace& space)
{
    group("Thread stack");

    Thread* t = new Thread(scheduler, space, counterA, false);

    // threadStartPoint is entered by "ret", so rsp must look like right after a call
    uintptr_t sp = t->getTCB().regs.rsp;
    CHECK(sp % 16 == 8);

    // The stack is a page of its own, given back when the thread is deleted
    uintptr_t page = sp & ~(uintptr_t)(pageSize2Mb - 1);
    CHECK(space.virtToPhys((void*)page) != nullptr);
    delete t;
    CHECK(space.virtToPhys((void*)page) == nullptr);
}

static int s_threadFrames {};

static void backtraceInThread(Scheduler& scheduler)
{
    uint64_t addresses[Backtrace::maxFrames];
    s_threadFrames = backtraceHere(addresses, Backtrace::maxFrames);
}

static void testThreadBacktrace(Scheduler& scheduler, AddressSpace& space)
{
    group("Backtrace in a thread");

    // backtraceHere <- backtraceInThread <- threadStartPoint, then rbp = 0:
    // the walk must stop at the top of the thread's stack
    s_threadFrames = -1;
    Thread* t = new Thread(scheduler, space, backtraceInThread);
    CHECK(waitForExit(scheduler, *t, 1000));
    CHECK(s_threadFrames >= 2 && s_threadFrames < Backtrace::maxFrames);
    delete t;
}

static void testRoundRobin(AddressSpace& space)
{
    group("Scheduler round robin (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    CHECK(scheduler.taskCount() == 1);

    Thread* a = new Thread(scheduler, space, counterA);
    Thread* b = new Thread(scheduler, space, counterB);
    CHECK(scheduler.taskCount() == 3);

    // Queue is [caller, A, B]: each yield from here runs A and B once
    for (int i = 0; i < 10; i++)
        scheduler.yield();
    CHECK(s_countA == 10);
    CHECK(s_countB == 10);

    // Returning from the entry point ends the thread and dequeues it
    CHECK(waitForExit(scheduler, *a, 1000));
    CHECK(waitForExit(scheduler, *b, 1000));
    CHECK(s_countA == rounds);
    CHECK(s_countB == rounds);
    CHECK(scheduler.taskCount() == 1);
    CHECK(s_frameAligned);

    delete a;
    delete b;
}

static volatile uint64_t s_sleptTicks {};

static void sleeper(Scheduler& scheduler)
{
    uint64_t t0 = scheduler.ticks();
    scheduler.sleep(50);
    s_sleptTicks = scheduler.ticks() - t0;
}

static void testSleepFakeTicks(AddressSpace& space)
{
    group("Scheduler sleep (fake ticks)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Thread* t = new Thread(scheduler, space, sleeper);

    // The sleeper runs, goes to sleep and the caller is the only runnable task
    scheduler.yield();
    CHECK(t->getTCB().state == ThreadState::Sleeping);

    ticks.advance(49);
    scheduler.yield();
    CHECK(t->getTCB().state == ThreadState::Sleeping);

    // Exactly at its wake tick it runs again and finishes
    ticks.advance(1);
    scheduler.yield();
    CHECK(t->getTCB().state == ThreadState::Terminated);
    CHECK(s_sleptTicks == 50);
    CHECK(scheduler.ticks() == 50);

    delete t;
}

static void testSleepRealTime(Scheduler& scheduler, AddressSpace& space)
{
    group("Scheduler sleep (kernel, PIT)");

    // Nothing else can run: the caller idles in hlt until the timer wakes it
    uint64_t t0 = scheduler.ticks();
    scheduler.sleep(20);
    uint64_t waited = scheduler.ticks() - t0;
    CHECK(waited >= 20);
    CHECK(waited < 120);

    // A sleeping thread is skipped until its time is up
    s_sleptTicks = 0;
    Thread* t = new Thread(scheduler, space, sleeper);
    CHECK(waitForExit(scheduler, *t, 1000));
    CHECK(s_sleptTicks >= 50);
    CHECK(s_sleptTicks < 150);
    delete t;
}

static volatile int s_blockStage {};

static void blocker(Scheduler& scheduler)
{
    s_blockStage = 1;
    scheduler.block();
    s_blockStage = 2;
}

static void testBlock(AddressSpace& space)
{
    group("Scheduler block (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Thread* t = new Thread(scheduler, space, blocker);
    for (int i = 0; i < 5; i++)
        scheduler.yield();

    // Blocked threads are skipped
    CHECK(s_blockStage == 1);
    CHECK(t->getTCB().state == ThreadState::Blocked);

    scheduler.unblock(t->getTCB());
    CHECK(waitForExit(scheduler, *t, 1000));
    CHECK(s_blockStage == 2);
    delete t;
}

static volatile bool s_ran {};

static void neverRuns(Scheduler&)
{
    s_ran = true;
}

static void testRemove(AddressSpace& space)
{
    group("Scheduler remove (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);

    // Deleting a queued thread before it ran takes it out of the queue
    Thread* t = new Thread(scheduler, space, neverRuns);
    CHECK(scheduler.taskCount() == 2);
    delete t;
    CHECK(scheduler.taskCount() == 1);

    for (int i = 0; i < 5; i++)
        scheduler.yield();
    CHECK(!s_ran);
}

static void testSchedulersAreIndependent(Scheduler& kernelScheduler, AddressSpace& space)
{
    group("Scheduler instances are independent");

    uint32_t kernelTasks = kernelScheduler.taskCount();

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    s_countA = 0;
    Thread* a = new Thread(scheduler, space, counterA);

    // The thread is queued on the test scheduler only
    CHECK(kernelScheduler.taskCount() == kernelTasks);
    kernelScheduler.yield();
    CHECK(s_countA == 0);

    scheduler.yield();
    CHECK(s_countA == 1);

    // Ticks are per scheduler
    uint64_t kernelTicks = kernelScheduler.ticks();
    ticks.advance(1000);
    CHECK(scheduler.ticks() == 1000);
    CHECK(kernelScheduler.ticks() - kernelTicks < 1000);

    CHECK(waitForExit(scheduler, *a, 1000));
    delete a;
}

// ------------------------------------------------------------
// Sync
//
// Each test runs on its own scheduler with a fake tick source; the test
// itself is task 0 and decides when the other threads run (yield).
// ------------------------------------------------------------

static void testAtomic()
{
    group("Atomic");
    using Sync::Atomic;

    Atomic<uint64_t> a(5);
    CHECK(a.load() == 5);
    a.store(7);
    CHECK(a.exchange(9) == 7);
    CHECK(a.load() == 9);

    uint64_t expected = 1;
    CHECK(!a.compareExchange(expected, 2)); // 9 != 1: expected gets the value
    CHECK(expected == 9);
    CHECK(a.compareExchange(expected, 2));
    CHECK(a.load() == 2);

    CHECK(a.fetchAdd(3) == 2);
    CHECK(a.fetchSub(5) == 5);
    CHECK(a.load() == 0);

    Atomic<uint8_t> small(255);
    small.fetchAdd(1);
    CHECK(small.load() == 0); // wraps like the plain type

    Atomic<bool> flag;
    CHECK(!flag.load());
    CHECK(!flag.exchange(true));
    CHECK(flag.load());

    int target = 0;
    Atomic<int*> pointer;
    CHECK(pointer.load() == nullptr);
    pointer.store(&target);
    CHECK(pointer.load() == &target);
}

static void testInterruptGuard()
{
    group("InterruptGuard");
    using Sync::InterruptGuard;

    CHECK(IOPort::interruptsEnabled()); // the tests run with interrupts on
    {
        InterruptGuard guard;
        CHECK(!IOPort::interruptsEnabled());
        {
            InterruptGuard inner;
            CHECK(!IOPort::interruptsEnabled());
        }
        CHECK(!IOPort::interruptsEnabled()); // the inner guard found them off
    }
    CHECK(IOPort::interruptsEnabled());

    // A guard taken with interrupts off leaves them off
    IOPort::disableInterrupts();
    {
        InterruptGuard guard;
    }
    CHECK(!IOPort::interruptsEnabled());
    IOPort::enableInterrupts();
}

static void testSpinlock()
{
    group("Spinlock");

    Sync::Spinlock lock;
    lock.lock();
    lock.unlock();
    lock.lock(); // free again after unlock
    lock.unlock();
    CHECK(true); // reached
}

static Sync::WaitQueue* s_waitQueue {};
static int s_wakeOrder[3] {};
static int s_wakeCount {};

template <int Id>
static void queueWaiter(Scheduler& scheduler)
{
    s_waitQueue->wait();
    s_wakeOrder[s_wakeCount++] = Id;
}

static void testWaitQueue(AddressSpace& space)
{
    group("WaitQueue (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Sync::WaitQueue queue(scheduler);
    s_waitQueue = &queue;
    s_wakeCount = 0;

    CHECK(queue.wakeOne() == nullptr);

    Thread* a = new Thread(scheduler, space, queueWaiter<0>);
    Thread* b = new Thread(scheduler, space, queueWaiter<1>);
    Thread* c = new Thread(scheduler, space, queueWaiter<2>);

    // One yield: a, b and c each run until they wait
    scheduler.yield();
    CHECK(queue.waiterCount() == 3);
    CHECK(b->getTCB().state == ThreadState::Blocked);

    // First in, first out
    CHECK(queue.wakeOne() == &a->getTCB());
    CHECK(s_wakeCount == 0); // woken is not running yet
    scheduler.yield();
    CHECK(s_wakeCount == 1 && s_wakeOrder[0] == 0);
    CHECK(queue.waiterCount() == 2);

    queue.wakeAll();
    CHECK(queue.isEmpty());
    CHECK(waitForExit(scheduler, *b, 1000));
    CHECK(waitForExit(scheduler, *c, 1000));
    CHECK(s_wakeCount == 3 && s_wakeOrder[1] == 1 && s_wakeOrder[2] == 2);
    CHECK(waitForExit(scheduler, *a, 1000));

    delete a;
    delete b;
    delete c;
}

static Sync::Semaphore* s_semaphore {};
static int s_taken {};

static void semaphoreTaker(Scheduler& scheduler)
{
    for (int i = 0; i < 3; i++) {
        s_semaphore->take();
        s_taken++;
    }
}

// What an interrupt handler would do: give from interrupt context
static void fakeInterruptHandler(void* semaphore)
{
    static_cast<Sync::Semaphore*>(semaphore)->give();
}

static void testSemaphore(AddressSpace& space)
{
    group("Semaphore (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);

    // Counting without waiting
    Sync::Semaphore counter(scheduler, 2, 3);
    CHECK(counter.tryTake());
    CHECK(counter.tryTake());
    CHECK(!counter.tryTake());
    CHECK(counter.give() && counter.give() && counter.give());
    CHECK(counter.count() == 3);
    CHECK(!counter.give()); // full: the unit is dropped
    CHECK(counter.count() == 3);

    // A taker waits at 0; give hands the unit over, the count stays 0
    Sync::Semaphore semaphore(scheduler, 0, 1);
    s_semaphore = &semaphore;
    s_taken = 0;
    Thread* t = new Thread(scheduler, space, semaphoreTaker);
    scheduler.yield();
    CHECK(s_taken == 0);
    CHECK(semaphore.waiterCount() == 1);

    CHECK(semaphore.give());
    CHECK(semaphore.count() == 0);
    CHECK(semaphore.waiterCount() == 0);
    scheduler.yield();
    CHECK(s_taken == 1);

    // From "interrupt context": interrupts off, no thread switch
    IOPort::disableInterrupts();
    fakeInterruptHandler(&semaphore);
    IOPort::enableInterrupts();
    scheduler.yield();
    CHECK(s_taken == 2);

    // The taker waits for its third unit again
    CHECK(semaphore.waiterCount() == 1);
    CHECK(semaphore.give());
    CHECK(waitForExit(scheduler, *t, 1000));
    CHECK(s_taken == 3);
    CHECK(semaphore.count() == 0);
    delete t;

    // Nobody waits: the unit is counted, up to max
    CHECK(semaphore.give());
    CHECK(semaphore.count() == 1);
    CHECK(!semaphore.give());
}

// Bounded buffer: the classic producer/consumer with two semaphores
static constexpr int bufferSize = 4;
static constexpr int itemCount = 20;
static int s_buffer[bufferSize] {};
static Sync::Semaphore* s_freeSlots {};
static Sync::Semaphore* s_filledSlots {};
static int s_consumedSum {};
static bool s_consumedInOrder {};

static void producer(Scheduler& scheduler)
{
    for (int i = 1; i <= itemCount; i++) {
        s_freeSlots->take();
        s_buffer[i % bufferSize] = i;
        s_filledSlots->give();
    }
}

static void consumer(Scheduler& scheduler)
{
    for (int i = 1; i <= itemCount; i++) {
        s_filledSlots->take();
        int value = s_buffer[i % bufferSize];
        s_consumedInOrder = s_consumedInOrder && value == i;
        s_consumedSum += value;
        s_freeSlots->give();
        if (i % 3 == 0)
            scheduler.yield(); // let the producer fill the buffer
    }
}

static void testProducerConsumer(AddressSpace& space)
{
    group("Semaphore producer/consumer (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Sync::Semaphore freeSlots(scheduler, bufferSize, bufferSize);
    Sync::Semaphore filledSlots(scheduler, 0, bufferSize);
    s_freeSlots = &freeSlots;
    s_filledSlots = &filledSlots;
    s_consumedSum = 0;
    s_consumedInOrder = true;

    Thread* p = new Thread(scheduler, space, producer);
    Thread* c = new Thread(scheduler, space, consumer);
    CHECK(waitForExit(scheduler, *p, 1000));
    CHECK(waitForExit(scheduler, *c, 1000));

    CHECK(s_consumedInOrder);
    CHECK(s_consumedSum == itemCount * (itemCount + 1) / 2);
    CHECK(freeSlots.count() == bufferSize);
    CHECK(filledSlots.count() == 0);
    delete p;
    delete c;
}

static Sync::Mutex* s_mutex {};
static int s_mutexLog[4] {};
static int s_mutexLogCount {};

// Holds the mutex across yields: without it, the two would interleave
template <int Id>
static void mutexUser(Scheduler& scheduler)
{
    s_mutex->lock();
    s_mutexLog[s_mutexLogCount++] = Id * 10; // in
    for (int i = 0; i < 3; i++)
        scheduler.yield();
    s_mutexLog[s_mutexLogCount++] = Id * 10 + 1; // out
    s_mutex->unlock();
}

static void testMutex(AddressSpace& space)
{
    group("Mutex (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Sync::Mutex mutex(scheduler);
    s_mutex = &mutex;
    s_mutexLogCount = 0;

    CHECK(!mutex.isLocked());
    mutex.lock();
    CHECK(mutex.isOwnedByCurrent());
    CHECK(!mutex.tryLock());
    mutex.unlock();
    CHECK(!mutex.isLocked());
    CHECK(mutex.tryLock());
    mutex.unlock();

    // The test holds the mutex: both threads wait, in order
    mutex.lock();
    Thread* a = new Thread(scheduler, space, mutexUser<1>);
    Thread* b = new Thread(scheduler, space, mutexUser<2>);
    scheduler.yield();
    CHECK(s_mutexLogCount == 0);
    CHECK(a->getTCB().state == ThreadState::Blocked);
    CHECK(b->getTCB().state == ThreadState::Blocked);

    // unlock hands the mutex to the first waiter before it runs
    mutex.unlock();
    CHECK(mutex.isLocked() && !mutex.isOwnedByCurrent());

    CHECK(waitForExit(scheduler, *a, 1000));
    CHECK(waitForExit(scheduler, *b, 1000));
    CHECK(s_mutexLogCount == 4);
    CHECK(s_mutexLog[0] == 10 && s_mutexLog[1] == 11 && s_mutexLog[2] == 20 && s_mutexLog[3] == 21);
    CHECK(!mutex.isLocked());
    delete a;
    delete b;
}

static Sync::ConditionVariable* s_condition {};
static bool s_ready {};
static int s_wakeups {};
static int s_sawReady {};

static void conditionWaiter(Scheduler& scheduler)
{
    s_mutex->lock();
    while (!s_ready) {
        s_condition->wait(*s_mutex);
        s_wakeups++;
    }
    s_sawReady++;
    s_mutex->unlock();
}

static void testConditionVariable(AddressSpace& space)
{
    group("ConditionVariable (isolated)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    Sync::Mutex mutex(scheduler);
    Sync::ConditionVariable condition(scheduler);
    s_mutex = &mutex;
    s_condition = &condition;
    s_ready = false;
    s_wakeups = 0;
    s_sawReady = 0;

    Thread* a = new Thread(scheduler, space, conditionWaiter);
    Thread* b = new Thread(scheduler, space, conditionWaiter);
    scheduler.yield();
    CHECK(condition.waiterCount() == 2);
    CHECK(!mutex.isLocked()); // wait() gave the mutex back

    // A notify without the condition: the waiter checks again and waits again
    condition.notifyOne();
    scheduler.yield();
    CHECK(s_wakeups == 1);
    CHECK(condition.waiterCount() == 2);
    CHECK(s_sawReady == 0);

    // Woken waiters need the mutex back before wait() returns
    mutex.lock();
    s_ready = true;
    condition.notifyAll();
    scheduler.yield();
    CHECK(s_sawReady == 0);
    CHECK(mutex.isOwnedByCurrent());
    mutex.unlock();

    CHECK(waitForExit(scheduler, *a, 1000));
    CHECK(waitForExit(scheduler, *b, 1000));
    CHECK(s_sawReady == 2);
    CHECK(!mutex.isLocked());
    delete a;
    delete b;
}

static void testIdleKeepsInterruptState(Scheduler& scheduler)
{
    group("Scheduler idle keeps the interrupt state (kernel, PIT)");

    // A thread waiting under an InterruptGuard may make the scheduler idle in
    // "sti; hlt"; it must get the CPU back with interrupts off, as it left it
    IOPort::disableInterrupts();
    uint64_t t0 = scheduler.ticks();
    scheduler.sleep(3);
    CHECK(!IOPort::interruptsEnabled());
    CHECK(scheduler.ticks() - t0 >= 3);
    IOPort::enableInterrupts();
}

// ------------------------------------------------------------
// Keyboard
// ------------------------------------------------------------

// Feed bytes, return the characters of the completed events (0 = none)
static char feedKey(Drivers::ScancodeDecoder& decoder, uint8_t byte)
{
    Drivers::KeyEvent event {};
    if (!decoder.feed(byte, event))
        return 0;
    return event.character;
}

static void testScancodeDecoder()
{
    group("Scancode decoder (set 1, US)");
    Drivers::ScancodeDecoder decoder;
    Drivers::KeyEvent event {};

    // Make code: a press with a character; break code: a release without
    CHECK(decoder.feed(0x1E, event));
    CHECK(event.pressed && !event.extended && event.code == 0x1E && event.character == 'a');
    CHECK(decoder.feed(0x9E, event));
    CHECK(!event.pressed && event.code == 0x1E && event.character == 0);

    CHECK(feedKey(decoder, 0x02) == '1');
    CHECK(feedKey(decoder, 0x39) == ' ');
    CHECK(feedKey(decoder, 0x1C) == '\n');
    CHECK(feedKey(decoder, 0x0E) == '\b');
    CHECK(feedKey(decoder, 0x0F) == '\t');
    CHECK(feedKey(decoder, 0x35) == '/');
    CHECK(feedKey(decoder, 0x3B) == 0); // F1
    CHECK(feedKey(decoder, 0x47) == '7'); // keypad
    CHECK(feedKey(decoder, 0x58) == 0); // F12: beyond the table

    // Shift (left and right) while held
    CHECK(feedKey(decoder, 0x2A) == 0);
    CHECK(decoder.shift());
    CHECK(feedKey(decoder, 0x1E) == 'A');
    CHECK(feedKey(decoder, 0x02) == '!');
    CHECK(feedKey(decoder, 0x28) == '"');
    CHECK(feedKey(decoder, 0xAA) == 0);
    CHECK(!decoder.shift());
    feedKey(decoder, 0x36);
    CHECK(feedKey(decoder, 0x0D) == '+');
    feedKey(decoder, 0xB6);
    CHECK(feedKey(decoder, 0x0D) == '=');

    // Caps Lock toggles on press, only letters, Shift undoes it
    feedKey(decoder, 0x3A);
    feedKey(decoder, 0xBA);
    CHECK(decoder.capsLock());
    CHECK(feedKey(decoder, 0x10) == 'Q');
    CHECK(feedKey(decoder, 0x02) == '1');
    feedKey(decoder, 0x2A);
    CHECK(feedKey(decoder, 0x10) == 'q');
    CHECK(feedKey(decoder, 0x02) == '!');
    feedKey(decoder, 0xAA);
    feedKey(decoder, 0x3A);
    CHECK(!decoder.capsLock());

    // Ctrl+letter: control characters; right Ctrl is E0 1D
    feedKey(decoder, 0x1D);
    CHECK(feedKey(decoder, 0x2E) == 3); // Ctrl+C
    feedKey(decoder, 0x9D);
    CHECK(!decoder.ctrl());
    CHECK(!decoder.feed(0xE0, event)); // a prefix completes nothing
    CHECK(decoder.feed(0x1D, event) && event.extended);
    CHECK(decoder.ctrl());
    feedKey(decoder, 0xE0);
    feedKey(decoder, 0x9D);
    CHECK(!decoder.ctrl());

    // Extended keys: keypad Enter and / have characters, arrows do not
    feedKey(decoder, 0xE0);
    CHECK(feedKey(decoder, 0x1C) == '\n');
    feedKey(decoder, 0xE0);
    CHECK(feedKey(decoder, 0x35) == '/');
    feedKey(decoder, 0xE0);
    CHECK(decoder.feed(0x48, event) && event.extended && event.character == 0); // up arrow
    CHECK(feedKey(decoder, 0x48) == '8'); // the prefix is gone: keypad 8

    // Print Screen's fake shift (E0 2A) does not shift
    feedKey(decoder, 0xE0);
    feedKey(decoder, 0x2A);
    CHECK(!decoder.shift());

    // Pause (E1 1D 45 E1 9D C5) and controller answers are not keys
    const uint8_t pause[] = { 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5 };
    for (uint8_t byte : pause)
        CHECK(!decoder.feed(byte, event));
    CHECK(!decoder.ctrl());
    CHECK(!decoder.feed(0xFA, event));
    CHECK(!decoder.feed(0xFE, event));
    CHECK(feedKey(decoder, 0x1E) == 'a');
}

static void typeText(Drivers::PS2Keyboard& keyboard, const char* text)
{
    // Only lowercase letters and space, enough for the tests
    static const uint8_t s_letters[] = { 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C };
    for (; *text; text++) {
        uint8_t code = *text == ' ' ? 0x39 : s_letters[*text - 'a'];
        keyboard.handleScancode(code);
        keyboard.handleScancode(code | 0x80);
    }
}

static Drivers::PS2Keyboard* s_keyboard {};
static char s_line[8] {};

static void lineReader(Scheduler& scheduler)
{
    for (int i = 0; i < 3; i++)
        s_line[i] = s_keyboard->read();
}

static void testKeyboard(AddressSpace& space)
{
    group("PS/2 keyboard (fake interrupt controller)");

    FakeTickSource ticks;
    Scheduler scheduler(ticks);
    FakeInterruptController controller;
    Drivers::PS2Keyboard keyboard(controller, scheduler);
    CHECK(controller.attachedIrq == Drivers::Irq::Keyboard);
    CHECK(controller.unmaskedIrq == Drivers::Irq::Keyboard);
    CHECK(controller.attachedArg == &keyboard);

    char c = 0;
    CHECK(!keyboard.tryRead(c));
    typeText(keyboard, "hi");
    CHECK(keyboard.tryRead(c) && c == 'h');
    CHECK(keyboard.tryRead(c) && c == 'i');
    CHECK(!keyboard.tryRead(c));

    // A reader waits without spinning until the "interrupt" brings characters
    s_keyboard = &keyboard;
    memset(s_line, 0, sizeof(s_line));
    Thread* t = new Thread(scheduler, space, lineReader);
    scheduler.yield();
    CHECK(t->getTCB().state == ThreadState::Blocked);
    IOPort::disableInterrupts(); // as in the IRQ handler
    typeText(keyboard, "ok");
    IOPort::enableInterrupts();
    scheduler.yield();
    CHECK(s_line[0] == 'o' && s_line[1] == 'k');
    CHECK(t->getTCB().state == ThreadState::Blocked);
    typeText(keyboard, "x");
    CHECK(waitForExit(scheduler, *t, 1000));
    CHECK(s_line[2] == 'x');
    delete t;

    // Full buffer: new characters are dropped, the typed ones keep their order
    for (uint32_t i = 0; i < Drivers::PS2Keyboard::bufferSize + 5; i++)
        typeText(keyboard, i % 2 ? "b" : "a");
    CHECK(keyboard.droppedCount() == 5);
    bool inOrder = true;
    for (uint32_t i = 0; i < Drivers::PS2Keyboard::bufferSize; i++)
        inOrder = inOrder && keyboard.tryRead(c) && c == (i % 2 ? 'b' : 'a');
    CHECK(inOrder);
    CHECK(!keyboard.tryRead(c));

    // The IRQ handler the driver registered reads the data port; just check it is ours
    CHECK(controller.attachedHandler != nullptr);
}

// ------------------------------------------------------------
// Storage: block devices, MBR, FAT16, VFS
// ------------------------------------------------------------

static constexpr uint32_t sectorSize = Drivers::BlockDevice::sectorSize;

static void testBlockDevices()
{
    group("Block devices and the MBR (RAM disk)");

    static uint8_t s_disk[100 * sectorSize];
    memset(s_disk, 0, sizeof(s_disk));
    for (uint32_t sector = 0; sector < 100; sector++)
        s_disk[sector * sectorSize + 1] = (uint8_t)sector; // tag every sector

    Drivers::MemoryBlockDevice disk(s_disk, 100);
    uint8_t buffer[2 * sectorSize];
    CHECK(disk.readSectors(98, 2, buffer) && buffer[1] == 98 && buffer[sectorSize + 1] == 99);
    CHECK(!disk.readSectors(99, 2, buffer));
    CHECK(!disk.readSectors(UINT64_MAX, 1, buffer)); // no overflow in the range check

    // No 0x55AA: not an MBR
    Drivers::PartitionInfo partitions[Drivers::mbrPartitionCount];
    CHECK(Drivers::readPartitionTable(disk, partitions) == -1);

    // Two partitions, one FAT16
    auto setEntry = [](int index, uint8_t type, uint32_t first, uint32_t count) {
        uint8_t* entry = s_disk + 446 + (size_t)index * 16;
        entry[4] = type;
        memcpy(entry + 8, &first, 4);
        memcpy(entry + 12, &count, 4);
    };
    s_disk[510] = 0x55;
    s_disk[511] = 0xAA;
    setEntry(0, 0x06, 10, 50);
    setEntry(2, 0x83, 60, 40);
    CHECK(Drivers::readPartitionTable(disk, partitions) == 2);
    CHECK(partitions[0].type == 0x06 && partitions[0].firstLba == 10 && partitions[0].sectorCount == 50);
    CHECK(partitions[1].type == 0x83 && partitions[1].firstLba == 60);
    CHECK(Drivers::isFat16PartitionType(0x06) && Drivers::isFat16PartitionType(0x0E));
    CHECK(!Drivers::isFat16PartitionType(0x83) && !Drivers::isFat16PartitionType(0x0B));

    // A partition is a device of its own, from its sector 0
    Drivers::PartitionDevice partition(disk, 10, 50, "ramp1");
    CHECK(partition.sectorCount() == 50 && !strcmp(partition.name(), "ramp1"));
    CHECK(partition.readSectors(0, 1, buffer) && buffer[1] == 10);
    CHECK(partition.readSectors(49, 1, buffer) && buffer[1] == 59);
    CHECK(!partition.readSectors(49, 2, buffer));
    memset(buffer, 0xAB, sectorSize);
    CHECK(partition.writeSectors(5, 1, buffer));
    CHECK(s_disk[15 * sectorSize + 1] == 0xAB);

    // An entry beyond the end of the disk: not a partition table
    setEntry(3, 0x06, 90, 20);
    CHECK(Drivers::readPartitionTable(disk, partitions) == -1);
}

// A FAT16 volume built by hand: 1 reserved sector, 2 FATs of 17 sectors,
// 512 root entries (32 sectors), 4200 clusters of one sector
static constexpr uint32_t fatClusters = 4200;
static constexpr uint32_t fatSectorsPerFat = 17;
static constexpr uint32_t fatRootSectors = 32;
static constexpr uint32_t fatFirstRoot = 1 + 2 * fatSectorsPerFat;
static constexpr uint32_t fatFirstData = fatFirstRoot + fatRootSectors;
static constexpr uint32_t fatTotalSectors = fatFirstData + fatClusters;
static constexpr uint32_t bigSize = 1300;
static const char s_helloText[] = "Hello, FAT16!";

static uint8_t bigByte(uint32_t i)
{
    return (uint8_t)(i * 7 + 3);
}

static void buildFat16(uint8_t* image, uint32_t totalSectors)
{
    memset(image, 0, (size_t)totalSectors * sectorSize);

    uint8_t* boot = image;
    boot[0] = 0xEB;
    uint16_t bytesPerSector = sectorSize;
    memcpy(boot + 11, &bytesPerSector, 2);
    boot[13] = 1; // sectors per cluster
    boot[14] = 1; // reserved sectors
    boot[16] = 2; // FATs
    uint16_t rootEntries = fatRootSectors * 16;
    memcpy(boot + 17, &rootEntries, 2);
    uint16_t total16 = (uint16_t)totalSectors;
    memcpy(boot + 19, &total16, 2);
    uint16_t spf = fatSectorsPerFat;
    memcpy(boot + 22, &spf, 2);
    boot[510] = 0x55;
    boot[511] = 0xAA;

    auto fat = [&](uint16_t cluster, uint16_t value) {
        for (int copy = 0; copy < 2; copy++)
            memcpy(image + (size_t)(1 + copy * fatSectorsPerFat) * sectorSize + (size_t)cluster * 2, &value, 2);
    };
    auto cluster = [&](uint16_t c) { return image + (size_t)(fatFirstData + c - 2) * sectorSize; };
    auto entry = [&](uint8_t* directory, int index, const char* name, uint8_t attributes, uint16_t first,
                     uint32_t size) {
        uint8_t* e = directory + (size_t)index * 32;
        memcpy(e, name, 11);
        e[11] = attributes;
        memcpy(e + 26, &first, 2);
        memcpy(e + 28, &size, 4);
    };
    fat(0, 0xFFF8);
    fat(1, 0xFFFF);

    uint8_t* root = image + (size_t)fatFirstRoot * sectorSize;
    entry(root, 0, "ATLOSTEST  ", 0x08, 0, 0); // volume label
    static const char s_longNamePart[11] = { 'A', 'h', 0, 'e', 0, 'l', 0, 'l', 0, 'o', 0 };
    entry(root, 1, s_longNamePart, 0x0F, 0, 0); // a long name part (UTF-16)
    entry(root, 2, "HELLO   TXT", 0x20, 2, sizeof(s_helloText) - 1);
    entry(root, 3, "\xE5OLD    TXT", 0x20, 11, 5); // deleted
    entry(root, 4, "SUBDIR     ", 0x10, 4, 0);
    entry(root, 5, "BIG     BIN", 0x20, 3, bigSize);
    entry(root, 6, "EMPTY   TXT", 0x20, 0, 0);
    entry(root, 7, "SHORT   BIN", 0x20, 8, 1500); // chain ends after one cluster
    entry(root, 8, "FREE    BIN", 0x20, 9, 1000); // chain runs into a free cluster
    entry(root, 9, "OUTSIDE BIN", 0x20, 9999, 10); // beyond the last cluster
    entry(root, 10, "LOOPDIR    ", 0x10, 10, 0); // a directory chain that loops

    memcpy(cluster(2), s_helloText, sizeof(s_helloText) - 1);
    fat(2, 0xFFFF);

    // BIG.BIN: clusters 3 -> 7 -> 5, not in order on purpose
    const uint16_t bigChain[] = { 3, 7, 5 };
    for (uint32_t i = 0; i < bigSize; i++)
        cluster(bigChain[i / sectorSize])[i % sectorSize] = bigByte(i);
    fat(3, 7);
    fat(7, 5);
    fat(5, 0xFFF8);

    uint8_t* subdir = cluster(4);
    entry(subdir, 0, ".          ", 0x10, 4, 0);
    entry(subdir, 1, "..         ", 0x10, 0, 0);
    entry(subdir, 2, "NESTED  TXT", 0x20, 6, 7);
    memcpy(cluster(6), "nested\n", 7);
    fat(4, 0xFFFF);
    fat(6, 0xFFFF);

    fat(8, 0xFFFF);
    fat(9, 0x0000);

    // Full of entries, no end marker, and its chain points at itself
    for (int i = 0; i < 16; i++) {
        char name[12];
        snprintf(name, sizeof(name), "F%02d     TXT", i);
        entry(cluster(10), i, name, 0x20, 0, 0);
    }
    fat(10, 10);
}

struct ListResult {
    char names[16][Fs::maxNameLength + 2];
    int count;
};

static void collectEntry(const Fs::FileInfo& entry, void* arg)
{
    auto* result = static_cast<ListResult*>(arg);
    if (result->count < 16)
        snprintf(result->names[result->count], sizeof(result->names[0]), "%s%s", entry.name,
            entry.isDirectory ? "/" : "");
    result->count++;
}

static void testFat16Names()
{
    group("FAT16 8.3 names");
    using Fs::Fat16;

    uint8_t shortName[11];
    CHECK(Fat16::toShortName("readme.txt", 10, shortName) && !memcmp(shortName, "README  TXT", 11));
    CHECK(Fat16::toShortName("a", 1, shortName) && !memcmp(shortName, "A          ", 11));
    CHECK(Fat16::toShortName("..", 2, shortName) && !memcmp(shortName, "..         ", 11));
    CHECK(Fat16::toShortName("dir/x", 3, shortName) && !memcmp(shortName, "DIR        ", 11)); // length counts
    CHECK(!Fat16::toShortName("toolongname", 11, shortName));
    CHECK(!Fat16::toShortName("a.toolong", 9, shortName));
    CHECK(!Fat16::toShortName("a.b.c", 5, shortName));
    CHECK(!Fat16::toShortName(".hidden", 7, shortName));

    char name[Fs::maxNameLength + 1];
    Fat16::fromShortName((const uint8_t*)"README  TXT", name);
    CHECK(!strcmp(name, "README.TXT"));
    Fat16::fromShortName((const uint8_t*)"SUBDIR     ", name);
    CHECK(!strcmp(name, "SUBDIR"));
    Fat16::fromShortName((const uint8_t*)"ABCDEFGHIJK", name);
    CHECK(!strcmp(name, "ABCDEFGH.IJK"));
}

static void testFat16()
{
    group("FAT16 (RAM disk)");

    auto* image = static_cast<uint8_t*>(kmalloc((size_t)fatTotalSectors * sectorSize));
    CHECK(image != nullptr);
    if (!image)
        return;
    buildFat16(image, fatTotalSectors);

    Drivers::MemoryBlockDevice volume(image, fatTotalSectors);
    Fs::Fat16 fat(volume);
    Fs::FileInfo info;
    CHECK(!fat.stat("HELLO.TXT", info)); // not mounted yet
    CHECK(fat.mount());
    CHECK(fat.clusterCount() == fatClusters && fat.clusterSize() == sectorSize);

    // Lookup: case insensitive, leading "/", deleted entries and labels skipped
    CHECK(fat.stat("HELLO.TXT", info) && !info.isDirectory && info.size == sizeof(s_helloText) - 1);
    CHECK(!strcmp(info.name, "HELLO.TXT"));
    CHECK(fat.stat("/hello.txt", info));
    CHECK(fat.stat("/", info) && info.isDirectory);
    CHECK(fat.stat("SUBDIR", info) && info.isDirectory && !strcmp(info.name, "SUBDIR"));
    CHECK(!fat.stat("NOPE.TXT", info));
    CHECK(!fat.stat("OLD.TXT", info));
    CHECK(!fat.stat("ATLOSTES.T", info));
    CHECK(!fat.stat("HELLO.TXT/X", info));

    // Reads: whole, from an offset, at and after the end
    char text[64] {};
    CHECK(fat.read("HELLO.TXT", 0, text, sizeof(text)) == (int64_t)sizeof(s_helloText) - 1);
    CHECK(!memcmp(text, s_helloText, sizeof(s_helloText) - 1));
    CHECK(fat.read("HELLO.TXT", 7, text, sizeof(text)) == 6 && !memcmp(text, "FAT16!", 6));
    CHECK(fat.read("HELLO.TXT", 13, text, sizeof(text)) == 0);
    CHECK(fat.read("HELLO.TXT", 1000, text, sizeof(text)) == 0);
    CHECK(fat.read("EMPTY.TXT", 0, text, sizeof(text)) == 0);
    CHECK(fat.read("SUBDIR", 0, text, sizeof(text)) == -1);
    CHECK(fat.read("NOPE.TXT", 0, text, sizeof(text)) == -1);

    // A file over three clusters that are not in order
    static uint8_t s_big[bigSize];
    CHECK(fat.read("BIG.BIN", 0, s_big, sizeof(s_big)) == bigSize);
    bool same = true;
    for (uint32_t i = 0; i < bigSize; i++)
        same = same && s_big[i] == bigByte(i);
    CHECK(same);
    CHECK(fat.read("BIG.BIN", 1000, s_big, 200) == 200); // across the cluster 2 -> 3 border
    same = true;
    for (uint32_t i = 0; i < 200; i++)
        same = same && s_big[i] == bigByte(1000 + i);
    CHECK(same);

    // Subdirectories, "..", and the listing
    CHECK(fat.read("SUBDIR/NESTED.TXT", 0, text, sizeof(text)) == 7 && !memcmp(text, "nested\n", 7));
    CHECK(fat.stat("subdir/../hello.txt", info) && info.size == sizeof(s_helloText) - 1);
    CHECK(fat.stat("SUBDIR/..", info) && info.isDirectory);

    ListResult list {};
    CHECK(fat.listDirectory("/", collectEntry, &list));
    CHECK(list.count == 8);
    CHECK(!strcmp(list.names[0], "HELLO.TXT") && !strcmp(list.names[1], "SUBDIR/") && !strcmp(list.names[7], "LOOPDIR/"));
    list = {};
    CHECK(fat.listDirectory("SUBDIR", collectEntry, &list));
    CHECK(list.count == 1 && !strcmp(list.names[0], "NESTED.TXT"));
    CHECK(!fat.listDirectory("HELLO.TXT", collectEntry, &list));

    // Broken volumes fail instead of returning garbage or hanging
    static uint8_t s_short[1500];
    CHECK(fat.read("SHORT.BIN", 0, s_short, 100) == 100); // the part that exists
    CHECK(fat.read("SHORT.BIN", 0, s_short, sizeof(s_short)) == -1);
    CHECK(fat.read("FREE.BIN", 0, s_short, 1000) == -1);
    CHECK(fat.read("OUTSIDE.BIN", 0, text, 10) == -1);
    list = {};
    CHECK(!fat.listDirectory("LOOPDIR", collectEntry, &list));

    // The VFS puts the volume under a name
    Fs::Vfs vfs;
    CHECK(vfs.mount("ram", fat));
    CHECK(!vfs.mount("ram", fat));
    CHECK(!vfs.mount("", fat));
    CHECK(!vfs.mount("a/b", fat));
    CHECK(!vfs.mount("a-very-long-mount-name", fat));
    CHECK(vfs.find("ram") == &fat && vfs.mountCount() == 1);
    CHECK(vfs.stat("/", info) && info.isDirectory);
    CHECK(vfs.stat("/ram", info) && info.isDirectory);
    CHECK(vfs.stat("/ram/SUBDIR/NESTED.TXT", info) && info.size == 7);
    CHECK(vfs.read("/ram/HELLO.TXT", 0, text, 5) == 5 && !memcmp(text, "Hello", 5));
    CHECK(!vfs.stat("/nope/HELLO.TXT", info));
    CHECK(vfs.read("/", 0, text, 5) == -1);
    list = {};
    CHECK(vfs.listDirectory("/", collectEntry, &list) && list.count == 1 && !strcmp(list.names[0], "ram/"));
    list = {};
    CHECK(vfs.listDirectory("/ram/SUBDIR", collectEntry, &list) && list.count == 1);

    // Not FAT16: a bad signature, or too few clusters (that is FAT12)
    image[511] = 0;
    Fs::Fat16 unsigned16(volume);
    CHECK(!unsigned16.mount());
    buildFat16(image, fatFirstData + 4000);
    Fs::Fat16 fat12(volume);
    CHECK(!fat12.mount());

    kfree(image);
}

static bool fatCopiesMatch(const uint8_t* image)
{
    return !memcmp(image + sectorSize, image + (size_t)(1 + fatSectorsPerFat) * sectorSize,
        (size_t)fatSectorsPerFat * sectorSize);
}

static bool readsBack(Fs::FileSystem& fs, const char* path, const void* expected, uint64_t size)
{
    static uint8_t s_readBack[4096];
    if (size > sizeof(s_readBack))
        return false;
    return fs.read(path, 0, s_readBack, sizeof(s_readBack)) == (int64_t)size && !memcmp(s_readBack, expected, size);
}

static void testFat16Write()
{
    group("FAT16 writing (RAM disk)");

    auto* image = static_cast<uint8_t*>(kmalloc((size_t)fatTotalSectors * sectorSize));
    CHECK(image != nullptr);
    if (!image)
        return;
    buildFat16(image, fatTotalSectors);

    Drivers::MemoryBlockDevice volume(image, fatTotalSectors);
    Fs::Fat16 fat(volume);
    CHECK(fat.mount());
    uint32_t freeAtStart = fat.freeClusterCount();
    CHECK(freeAtStart < fatClusters && freeAtStart > fatClusters - 20);

    // create: an empty file, once
    Fs::FileInfo info;
    CHECK(fat.create("new.txt"));
    CHECK(!fat.create("NEW.TXT"));
    CHECK(fat.stat("NEW.TXT", info) && info.size == 0 && !info.isDirectory);
    char text[16] {};
    CHECK(fat.read("NEW.TXT", 0, text, sizeof(text)) == 0);
    CHECK(fat.freeClusterCount() == freeAtStart);

    // write: append, overwrite, no holes
    CHECK(fat.write("NEW.TXT", 0, "abc", 3) == 3);
    CHECK(fat.write("NEW.TXT", 3, "def", 3) == 3);
    CHECK(fat.write("NEW.TXT", 1, "X", 1) == 1);
    CHECK(fat.write("NEW.TXT", 10, "late", 4) == -1);
    CHECK(readsBack(fat, "NEW.TXT", "aXcdef", 6));
    CHECK(fat.freeClusterCount() == freeAtStart - 1);

    // A file over several clusters, written in odd sized pieces
    static uint8_t s_data[3000];
    for (uint32_t i = 0; i < sizeof(s_data); i++)
        s_data[i] = (uint8_t)(i * 13 + 1);
    CHECK(fat.create("DATA.BIN"));
    for (uint32_t done = 0; done < sizeof(s_data); done += 333) {
        uint32_t chunk = sizeof(s_data) - done < 333 ? sizeof(s_data) - done : 333;
        CHECK(fat.write("DATA.BIN", done, s_data + done, chunk) == chunk);
    }
    CHECK(fat.stat("DATA.BIN", info) && info.size == sizeof(s_data));
    CHECK(readsBack(fat, "DATA.BIN", s_data, sizeof(s_data)));
    CHECK(fat.freeClusterCount() == freeAtStart - 1 - 6); // 3000 bytes: 6 clusters of 512
    CHECK(fatCopiesMatch(image));

    // An existing file keeps its other files intact
    CHECK(readsBack(fat, "HELLO.TXT", s_helloText, sizeof(s_helloText) - 1));
    CHECK(fat.write("SUBDIR", 0, "x", 1) == -1);
    CHECK(fat.write("NOPE.TXT", 0, "x", 1) == -1);

    // remove gives the clusters back
    CHECK(fat.remove("DATA.BIN"));
    CHECK(!fat.stat("DATA.BIN", info));
    CHECK(!fat.remove("DATA.BIN"));
    CHECK(fat.remove("NEW.TXT"));
    CHECK(fat.freeClusterCount() == freeAtStart);
    CHECK(!fat.remove("SUBDIR")); // a directory
    CHECK(!fat.remove("/"));

    // mkdir: "." and "..", files inside, growing past one cluster (16 entries)
    CHECK(fat.makeDirectory("NEWDIR"));
    CHECK(!fat.makeDirectory("NEWDIR"));
    CHECK(fat.stat("NEWDIR", info) && info.isDirectory);
    CHECK(fat.stat("NEWDIR/..", info) && info.isDirectory && !strcmp(info.name, "/"));
    CHECK(fat.makeDirectory("NEWDIR/INNER"));
    CHECK(fat.create("NEWDIR/INNER/DEEP.TXT"));
    CHECK(fat.write("NEWDIR/INNER/../INNER/DEEP.TXT", 0, "deep", 4) == 4);
    CHECK(readsBack(fat, "NEWDIR/INNER/DEEP.TXT", "deep", 4));
    CHECK(fat.stat("NEWDIR/INNER/..", info) && info.isDirectory);
    CHECK(fat.stat("NEWDIR/INNER/../INNER/DEEP.TXT", info) && info.size == 4);

    for (int i = 0; i < 20; i++) {
        char path[32];
        snprintf(path, sizeof(path), "NEWDIR/F%02d.TXT", i);
        CHECK(fat.create(path));
    }
    ListResult list {};
    CHECK(fat.listDirectory("NEWDIR", collectEntry, &list));
    CHECK(list.count == 21); // INNER + 20 files
    CHECK(fat.stat("NEWDIR/F19.TXT", info));

    // rmdir: only empty directories, never the root, ".", ".."
    CHECK(!fat.removeDirectory("NEWDIR"));
    CHECK(!fat.removeDirectory("/"));
    CHECK(!fat.removeDirectory("NEWDIR/.."));
    CHECK(!fat.removeDirectory("NEWDIR/INNER/."));
    CHECK(!fat.removeDirectory("HELLO.TXT"));
    CHECK(!fat.create("NEWDIR/.."));
    CHECK(fat.remove("NEWDIR/INNER/DEEP.TXT"));
    CHECK(fat.removeDirectory("NEWDIR/INNER"));
    for (int i = 0; i < 20; i++) {
        char path[32];
        snprintf(path, sizeof(path), "NEWDIR/F%02d.TXT", i);
        CHECK(fat.remove(path));
    }
    CHECK(fat.removeDirectory("NEWDIR"));
    CHECK(!fat.stat("NEWDIR", info));
    CHECK(fat.freeClusterCount() == freeAtStart);

    // Names and parents that do not work
    CHECK(!fat.create("NOPE/X.TXT"));
    CHECK(!fat.create("BAD.NAME.X"));
    CHECK(!fat.create("TOOLONGNAME.TXT"));
    CHECK(!fat.create("HELLO.TXT/X"));
    CHECK(!fat.makeDirectory(""));

    // A full volume: the write stops at the last cluster, remove gives all back
    uint32_t chunkSize = 64 * 1024;
    auto* chunk = static_cast<uint8_t*>(kcalloc(1, chunkSize));
    CHECK(fat.create("HUGE.BIN"));
    uint64_t written = 0;
    for (int i = 0; i < 64; i++) {
        int64_t n = fat.write("HUGE.BIN", written, chunk, chunkSize);
        if (n <= 0)
            break;
        written += n;
    }
    CHECK(fat.freeClusterCount() == 0);
    CHECK(written == (uint64_t)freeAtStart * sectorSize);
    CHECK(fat.write("HUGE.BIN", written, chunk, 1) == -1);
    CHECK(!fat.makeDirectory("NOSPACE"));
    CHECK(fat.remove("HUGE.BIN"));
    CHECK(fat.freeClusterCount() == freeAtStart);
    kfree(chunk);

    // A full root directory can not grow (512 entries, 10 in use; deleted ones are reused)
    int created = 0;
    for (int i = 0; i < 600; i++) {
        char path[16];
        snprintf(path, sizeof(path), "R%03d.TXT", i);
        if (!fat.create(path))
            break;
        created++;
    }
    CHECK(created == 512 - 10);
    CHECK(fatCopiesMatch(image));

    // The VFS forwards writes
    Fs::Vfs vfs;
    CHECK(vfs.mount("ram", fat));
    CHECK(vfs.remove("/ram/R000.TXT"));
    CHECK(vfs.create("/ram/V.TXT"));
    CHECK(vfs.write("/ram/V.TXT", 0, "vfs", 3) == 3);
    CHECK(vfs.makeDirectory("/ram/SUBDIR/VDIR"));
    CHECK(vfs.removeDirectory("/ram/SUBDIR/VDIR"));
    CHECK(!vfs.create("/ram"));
    CHECK(!vfs.makeDirectory("/"));
    CHECK(!vfs.create("/nope/X.TXT"));

    kfree(image);
}

// The test disk, see scripts/make-disk-image.sh
static const char s_diskHello[] = "Hello from the atlOS disk!\n";
static const char s_diskReadme[] = "atlOS test disk: a file in a subdirectory.\n";
static constexpr uint32_t diskBigLines = 2000;
static constexpr uint32_t diskBigLineLength = 11; // "line 00001\n"

static void testAtaDisk(Fs::Vfs& vfs)
{
    group("ATA disk (test disk)");

    Drivers::ATAPioDevice disk(Drivers::ATAPioDevice::Channel::Primary, Drivers::ATAPioDevice::Drive::Master, "test");
    if (!disk.isPresent()) {
        printf("[SELFTEST] skipped: no ATA disk (the test scripts attach one)\n");
        return;
    }
    CHECK(disk.sectorCount() >= 2048 + fatTotalSectors);
    CHECK(disk.model()[0] != '\0');

    Drivers::PartitionInfo partitions[Drivers::mbrPartitionCount];
    CHECK(Drivers::readPartitionTable(disk, partitions) == 1);
    CHECK(partitions[0].firstLba == 2048 && Drivers::isFat16PartitionType(partitions[0].type));

    // Write and read back sector 1: unused, between the MBR and the partition
    static uint8_t s_original[sectorSize];
    static uint8_t s_pattern[2 * sectorSize];
    static uint8_t s_check[2 * sectorSize];
    CHECK(disk.readSectors(1, 1, s_original));
    for (uint32_t i = 0; i < sizeof(s_pattern); i++)
        s_pattern[i] = (uint8_t)(i ^ 0x5A);
    CHECK(disk.writeSectors(1, 1, s_pattern));
    CHECK(disk.readSectors(0, 2, s_check)); // two sectors in one command
    CHECK(!memcmp(s_check + sectorSize, s_pattern, sectorSize));
    CHECK(s_check[510] == 0x55 && s_check[511] == 0xAA); // the MBR is untouched
    CHECK(disk.writeSectors(1, 1, s_original));
    CHECK(!disk.readSectors(disk.sectorCount(), 1, s_check));

    // The files storage mounted at boot
    CHECK(vfs.find("disk0") != nullptr);
    Fs::FileInfo info;
    char text[64] {};
    CHECK(vfs.stat("/disk0/HELLO.TXT", info) && info.size == sizeof(s_diskHello) - 1);
    CHECK(vfs.read("/disk0/HELLO.TXT", 0, text, sizeof(text)) == (int64_t)sizeof(s_diskHello) - 1);
    CHECK(!memcmp(text, s_diskHello, sizeof(s_diskHello) - 1));
    CHECK(vfs.read("/disk0/docs/readme.txt", 0, text, sizeof(text)) == (int64_t)sizeof(s_diskReadme) - 1);
    CHECK(!memcmp(text, s_diskReadme, sizeof(s_diskReadme) - 1));

    // A file over many clusters: every line in order
    uint64_t bigSize = (uint64_t)diskBigLines * diskBigLineLength;
    CHECK(vfs.stat("/disk0/BIG.TXT", info) && info.size == bigSize);
    auto* big = static_cast<char*>(kmalloc(bigSize));
    CHECK(vfs.read("/disk0/BIG.TXT", 0, big, bigSize) == (int64_t)bigSize);
    bool lines = true;
    for (uint32_t i = 0; i < diskBigLines; i++) {
        char expected[16];
        snprintf(expected, sizeof(expected), "line %05u\n", i + 1);
        lines = lines && !memcmp(big + (size_t)i * diskBigLineLength, expected, diskBigLineLength);
    }
    CHECK(lines);
    kfree(big);
    CHECK(vfs.read("/disk0/BIG.TXT", 1234ULL * diskBigLineLength, text, diskBigLineLength) == diskBigLineLength);
    CHECK(!memcmp(text, "line 01235\n", diskBigLineLength));

    ListResult list {};
    CHECK(vfs.listDirectory("/disk0", collectEntry, &list));
    CHECK(list.count == 3);

    // Writing. The files stay: scripts/run-selftest.sh checks them on the host
    // afterwards with fsck.fat and mtools (another FAT implementation)
    CHECK(vfs.makeDirectory("/disk0/KERNEL"));
    CHECK(vfs.create("/disk0/KERNEL/NOTE.TXT"));
    static const char s_note[] = "written by atlOS\n";
    CHECK(vfs.write("/disk0/KERNEL/NOTE.TXT", 0, s_note, sizeof(s_note) - 1) == (int64_t)sizeof(s_note) - 1);

    // 1000 lines "data 00001\n", appended in pieces that do not match sectors
    CHECK(vfs.create("/disk0/KERNEL/DATA.TXT"));
    static char s_lines[1000 * diskBigLineLength + 1];
    for (uint32_t i = 0; i < 1000; i++)
        snprintf(s_lines + (size_t)i * diskBigLineLength, diskBigLineLength + 1, "data %05u\n", i + 1);
    uint64_t total = 1000ULL * diskBigLineLength;
    bool appended = true;
    for (uint64_t done = 0; done < total; done += 333) {
        uint64_t chunk = total - done < 333 ? total - done : 333;
        appended = appended && vfs.write("/disk0/KERNEL/DATA.TXT", done, s_lines + done, chunk) == (int64_t)chunk;
    }
    CHECK(appended);
    CHECK(vfs.stat("/disk0/KERNEL/DATA.TXT", info) && info.size == total);
    auto* back = static_cast<char*>(kmalloc(total));
    CHECK(vfs.read("/disk0/KERNEL/DATA.TXT", 0, back, total) == (int64_t)total && !memcmp(back, s_lines, total));
    kfree(back);

    // Created and removed again: must leave no trace for fsck
    CHECK(vfs.makeDirectory("/disk0/KERNEL/TMP"));
    CHECK(vfs.create("/disk0/KERNEL/TMP/GONE.TXT"));
    CHECK(vfs.write("/disk0/KERNEL/TMP/GONE.TXT", 0, s_lines, 5000) == 5000);
    CHECK(vfs.remove("/disk0/KERNEL/TMP/GONE.TXT"));
    CHECK(vfs.removeDirectory("/disk0/KERNEL/TMP"));
}

int run(PhysicalMemoryManager& pmm, AddressSpace& space, Threading::Scheduler& scheduler, Fs::Vfs& vfs)
{
    printf("[SELFTEST] start\n");

    testPmmBlocks();
    testPmmFrames();
    testPmmMultiboot();
    testPmmKernel(pmm);
    testVmm(pmm, space);
    testHeap();
    testInterrupts(scheduler);
    testUninitialized();
    testPitWithFakeController(scheduler);
    testConsole();
    testVgaConsole();
    testSerialPortMissing();
    testCpu();
    testFaultDecoding();
    testTaskStateSegment();
    testBacktrace();
    testThreadStack(scheduler, space);
    testThreadBacktrace(scheduler, space);
    testRoundRobin(space);
    testSleepFakeTicks(space);
    testSleepRealTime(scheduler, space);
    testBlock(space);
    testRemove(space);
    testSchedulersAreIndependent(scheduler, space);
    testAtomic();
    testInterruptGuard();
    testSpinlock();
    testWaitQueue(space);
    testSemaphore(space);
    testProducerConsumer(space);
    testMutex(space);
    testConditionVariable(space);
    testIdleKeepsInterruptState(scheduler);
    testScancodeDecoder();
    testKeyboard(space);
    testBlockDevices();
    testFat16Names();
    testFat16();
    testFat16Write();
    testAtaDisk(vfs);

    printf("[SELFTEST] done: %d passed, %d failed\n", s_passed, s_failed);
    return s_failed;
}

} // namespace SelfTest
