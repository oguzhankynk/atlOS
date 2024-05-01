/**
 * @file idt.cpp
 * @author Oguzhan KAYNAK
 * @brief Source file for Interrupt Descriptor Table(IDT) functions.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <arch/i386/IDT/idt.hpp>
#include <arch/i386/cpu/cpu.hpp>

/**
 * @brief 'exception' functions defined in ISR.S
 * 
 */
extern "C" void exception_0();
extern "C" void exception_1();
extern "C" void exception_2();
extern "C" void exception_3();
extern "C" void exception_4();
extern "C" void exception_5();
extern "C" void exception_6();
extern "C" void exception_7();
extern "C" void exception_8();
extern "C" void exception_9();
extern "C" void exception_10();
extern "C" void exception_11();
extern "C" void exception_12();
extern "C" void exception_13();
extern "C" void exception_14();
extern "C" void exception_15();
extern "C" void exception_16();
extern "C" void exception_17();
extern "C" void exception_18();
extern "C" void exception_19();
extern "C" void exception_20();
extern "C" void exception_21();
extern "C" void exception_22();


/**
 * @brief Initializes the IDT.
 * 
 */
void IDT::Init()
{
    setEntry(0 , reinterpret_cast<uint64_t>(&exception_0 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(1 , reinterpret_cast<uint64_t>(&exception_1 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(2 , reinterpret_cast<uint64_t>(&exception_2 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(3 , reinterpret_cast<uint64_t>(&exception_3 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(4 , reinterpret_cast<uint64_t>(&exception_4 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(5 , reinterpret_cast<uint64_t>(&exception_5 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(6 , reinterpret_cast<uint64_t>(&exception_6 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(7 , reinterpret_cast<uint64_t>(&exception_7 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(8 , reinterpret_cast<uint64_t>(&exception_8 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(9 , reinterpret_cast<uint64_t>(&exception_9 ), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(10, reinterpret_cast<uint64_t>(&exception_10), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(11, reinterpret_cast<uint64_t>(&exception_11), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(12, reinterpret_cast<uint64_t>(&exception_12), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(13, reinterpret_cast<uint64_t>(&exception_13), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(14, reinterpret_cast<uint64_t>(&exception_14), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(16, reinterpret_cast<uint64_t>(&exception_16), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(17, reinterpret_cast<uint64_t>(&exception_17), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(18, reinterpret_cast<uint64_t>(&exception_18), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(19, reinterpret_cast<uint64_t>(&exception_19), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(20, reinterpret_cast<uint64_t>(&exception_20), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);
    setEntry(21, reinterpret_cast<uint64_t>(&exception_21), static_cast<uint16_t>(SegmentSelect::KERNELCODE), 0x8E);

    // Set the IDTR stru)cture
    idtr.limit = sizeof(idt) - 1;
    idtr.base = reinterpret_cast<uint64_t>(&idt);

    load();
}

/**
 * @brief Load the IDT into the CPU
 *
 */
void IDT::load()
{
    // Load the IDT using the `lidt` assembly instruction
    __asm__ volatile("lidt %0" : : "m"(idtr));
}

/**
 * @brief Sets an entry in the IDT.
 *
 * @param index The index of the IDT entry to set.
 * @param handler The address of the interrupt handler.
 * @param selector The code segment selector.
 * @param type_attr The type and attributes of the entry.
 *
 * Configures an entry in the IDT with the provided handler address, code segment
 * selector, and type attributes.
 */
void IDT::setEntry(int index, uint64_t handler, uint16_t selector, uint8_t type_attr)
{
    // Configure the IDT entry
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type_attr;
    idt[index].offset_high = (handler >> 16) & 0xFFFF;
}

/**
 * @brief Exception handler
 * @param regs The status of registers at exception time
 */
extern "C" void isr_exception_handler(X86ExceptionInfo regs) 
{
    const char *int_str;

    switch (static_cast<ExternalInterrupts>(regs.int_no))
    {
        case ExternalInterrupts::DivideError:
            int_str = "DivideError";
            break;

        case ExternalInterrupts::DebugException:
            int_str = "DebugException";
            break;
            
        case ExternalInterrupts::NMIInterrupt:
            int_str = "NMIInterrupt";
            break;
            
        case ExternalInterrupts::Breakpoint:
            int_str = "Breakpoint";
            break;
            
        case ExternalInterrupts::Overflow:
            int_str = "Overflow";
            break;

        case ExternalInterrupts::BOUNDRangeExceeded:
            int_str = "BOUNDRangeExceeded";
            break;

        case ExternalInterrupts::InvalidOpcode:
            int_str = "InvalidOpcode";
            break;

        case ExternalInterrupts::DeviceNotAvailable:
            int_str = "DeviceNotAvailable";
            break;

        case ExternalInterrupts::DoubleFault:
            int_str = "DoubleFault";
            break;

        case ExternalInterrupts::CoprocessorSegmentOverrun:
            int_str = "CoprocessorSegmentOverrun";
            break;

        case ExternalInterrupts::reserved:
            int_str = "reserved";
            break;

        case ExternalInterrupts::InvalidTSS:
            int_str = "InvalidTSS";
            break;

        case ExternalInterrupts::SegmentNotPresent:
            int_str = "SegmentNotPresent";
            break;

        case ExternalInterrupts::StackSegmentFault:
            int_str = "StackSegmentFault";
            break;

        case ExternalInterrupts::GeneralProtection:
            int_str = "GeneralProtection";
            break;

        case ExternalInterrupts::PageFault:
            int_str = "PageFault";
            break;

        case ExternalInterrupts::IntelReserved:
            int_str = "IntelReserved";
            break;

        case ExternalInterrupts::x87_FPU_FloatingPointError:
            int_str = "x87_FPU_FloatingPointError";
            break;

        case ExternalInterrupts::AlignmentCheck:
            int_str = "AlignmentCheck";
            break;

        case ExternalInterrupts::MachineCheck:
            int_str = "MachineCheck";
            break;

        case ExternalInterrupts::SIMD_FloatingPointException:
            int_str = "SIMD_FloatingPointException";
            break;

        case ExternalInterrupts::VirtualizationException:
            int_str = "VirtualizationException";
            break;

        case ExternalInterrupts::ControlProtectionException:
            int_str = "ControlProtectionException";
            break;

    default:
        int_str = "OtherException";
        break;
    }

    printf("Exception: %s\n",int_str);

    regs.print();
    while(1);
}

