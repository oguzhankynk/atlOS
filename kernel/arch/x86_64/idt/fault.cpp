/**
 * @file fault.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding and reporting of CPU exceptions (vectors 0..31).
 */

#include <arch/x86_64/idt/fault.hpp>

#include <stdio.h>

#include <arch/x86_64/idt/idt.hpp>
#include <core/panic.hpp>

namespace Fault {

using IDT::ExternalInterrupts;

static constexpr uint32_t exceptionCount = 32;

static const char* const s_exceptionNames[exceptionCount] = {
    "Divide error", // 0
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND range exceeded", // 5
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS", // 10
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved", // 15
    "x87 floating point error",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception",
    "Virtualization exception", // 20
    "Control protection exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved", // 25
    "Reserved",
    "Reserved",
    "Hypervisor injection",
    "VMM communication",
    "Security exception", // 30
    "Reserved",
};

PageFaultError decodePageFault(uint64_t errorCode)
{
    PageFaultError error;
    error.present = errorCode & (1 << 0);
    error.write = errorCode & (1 << 1);
    error.user = errorCode & (1 << 2);
    error.reservedBit = errorCode & (1 << 3);
    error.instructionFetch = errorCode & (1 << 4);
    return error;
}

SelectorError decodeSelectorError(uint64_t errorCode)
{
    // Bit 0: EXT, bit 1: IDT, bit 2: TI (LDT instead of GDT, only without IDT), bits 3..15: index
    SelectorError error;
    error.external = errorCode & (1 << 0);
    if (errorCode & (1 << 1))
        error.table = DescriptorTable::IDT;
    else if (errorCode & (1 << 2))
        error.table = DescriptorTable::LDT;
    else
        error.table = DescriptorTable::GDT;
    error.index = (errorCode >> 3) & 0x1FFF;
    return error;
}

const char* exceptionName(uint64_t vector)
{
    if (vector >= exceptionCount)
        return nullptr;
    return s_exceptionNames[vector];
}

static uint64_t readCr2()
{
    uint64_t value;
    __asm__ volatile("mov %%cr2, %0"
                     : "=r"(value));
    return value;
}

static const char* tableName(DescriptorTable table)
{
    switch (table) {
    case DescriptorTable::GDT:
        return "GDT";
    case DescriptorTable::IDT:
        return "IDT";
    case DescriptorTable::LDT:
        return "LDT";
    }
    return "?";
}

static void printPageFault(uint64_t errorCode)
{
    // CR2 holds the address the access was made to
    uint64_t address = readCr2();
    PageFaultError error = decodePageFault(errorCode);

    const char* access = error.instructionFetch ? "instruction fetch" : (error.write ? "write" : "read");
    printf("CR2=0x%016lx (%s %s, %s)\n", address, error.user ? "user" : "kernel", access,
        error.present ? "protection violation" : "page not present");
    if (error.reservedBit)
        printf("a page table entry has a reserved bit set\n");
}

static void printSelectorError(uint64_t errorCode)
{
    if (!errorCode) {
        printf("not caused by a segment selector\n");
        return;
    }

    SelectorError error = decodeSelectorError(errorCode);
    printf("selector: %s index %u%s\n", tableName(error.table), error.index,
        error.external ? " (while delivering an external event)" : "");
}

static void printRegisters(const InterruptFrame& regs)
{
    printf("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx\n",
        regs.rax, regs.rbx, regs.rcx, regs.rdx);
    printf("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx\n",
        regs.rsi, regs.rdi, regs.rbp);
    printf("R8 =0x%016lx R9 =0x%016lx R10=0x%016lx R11=0x%016lx\n",
        regs.r8, regs.r9, regs.r10, regs.r11);
    printf("R12=0x%016lx R13=0x%016lx R14=0x%016lx R15=0x%016lx\n",
        regs.r12, regs.r13, regs.r14, regs.r15);
    printf("RIP=0x%016lx CS=0x%04lx RFLAGS=0x%016lx\n",
        regs.rip, regs.cs, regs.rflags);
    printf("RSP=0x%016lx SS=0x%04lx\n", regs.rsp, regs.ss);
}

void printReport(const InterruptFrame& frame)
{
    printf("\n==== EXCEPTION %lu: %s%s ====\n", frame.intNo, exceptionName(frame.intNo),
        isFromUserMode(frame) ? " (in user mode)" : "");
    printf("error code 0x%lx\n", frame.errCode);

    switch (static_cast<ExternalInterrupts>(frame.intNo)) {
    case ExternalInterrupts::PageFault:
        printPageFault(frame.errCode);
        break;

    case ExternalInterrupts::InvalidTSS:
    case ExternalInterrupts::SegmentNotPresent:
    case ExternalInterrupts::StackSegmentFault:
    case ExternalInterrupts::GeneralProtection:
        printSelectorError(frame.errCode);
        break;

    case ExternalInterrupts::DoubleFault:
        // Often a kernel stack overflow: the first fault could not push its frame
        printf("running on the double fault stack (IST%u); the interrupted stack may be broken\n",
            (unsigned)IDT::entry(frame.intNo).ist);
        break;

    default:
        break;
    }

    printRegisters(frame);
}

// The table the linker collects from every .extable section (see msr.S)
struct ExceptionTableEntry {
    uint64_t instruction;
    uint64_t resume;
};

extern "C" const ExceptionTableEntry __extable_start[];
extern "C" const ExceptionTableEntry __extable_end[];

uint32_t exceptionTableSize()
{
    return __extable_end - __extable_start;
}

bool fixup(InterruptFrame& frame)
{
    for (const ExceptionTableEntry* entry = __extable_start; entry < __extable_end; entry++) {
        if (entry->instruction == frame.rip) {
            frame.rip = entry->resume;
            return true;
        }
    }
    return false;
}

void reportException(const InterruptFrame& frame)
{
    printReport(frame);

    // The backtrace starts at the faulting instruction
    kernelPanicFrom(exceptionName(frame.intNo), frame.rip, frame.rbp);
}

} // namespace Fault
