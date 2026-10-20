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
#include <drivers/interrupt_controller.hpp>
#include <drivers/pit.hpp>
#include <drivers/serial.hpp>
#include <drivers/tick_source.hpp>
#include <drivers/vga_console.hpp>
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

int run(PhysicalMemoryManager& pmm, AddressSpace& space, Threading::Scheduler& scheduler)
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

    printf("[SELFTEST] done: %d passed, %d failed\n", s_passed, s_failed);
    return s_failed;
}

} // namespace SelfTest
