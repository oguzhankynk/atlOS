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

#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/idt/idt.hpp>

namespace IDT {

#define INT(x) extern "C" void INT_##x();
#define INT_RESERVED(x)

#define IDT_TABLE    \
    INT(0)           \
    INT(1)           \
    INT(2)           \
    INT(3)           \
    INT(4)           \
    INT(5)           \
    INT(6)           \
    INT(7)           \
    INT(8)           \
    INT_RESERVED(9)  \
    INT(10)          \
    INT(11)          \
    INT(12)          \
    INT(13)          \
    INT(14)          \
    INT_RESERVED(15) \
    INT(16)          \
    INT(17)          \
    INT(18)          \
    INT(19)          \
    INT(20)          \
    INT(21)          \
    INT_RESERVED(22) \
    INT_RESERVED(23) \
    INT_RESERVED(24) \
    INT_RESERVED(25) \
    INT_RESERVED(26) \
    INT_RESERVED(27) \
    INT_RESERVED(28) \
    INT_RESERVED(29) \
    INT_RESERVED(30) \
    INT_RESERVED(31) \
    INT(32)          \
    INT(33)          \
    INT(34)          \
    INT(35)          \
    INT(36)          \
    INT(37)          \
    INT(38)          \
    INT(39)          \
    INT(40)          \
    INT(41)          \
    INT(42)          \
    INT(43)          \
    INT(44)          \
    INT(45)          \
    INT(46)          \
    INT(47)          \
    INT(48)          \
    INT(49)          \
    INT(50)          \
    INT(51)          \
    INT(52)          \
    INT(53)          \
    INT(54)          \
    INT(55)          \
    INT(56)          \
    INT(57)          \
    INT(58)          \
    INT(59)          \
    INT(60)          \
    INT(61)          \
    INT(62)          \
    INT(63)          \
    INT(64)          \
    INT(65)          \
    INT(66)          \
    INT(67)          \
    INT(68)          \
    INT(69)          \
    INT(70)          \
    INT(71)          \
    INT(72)          \
    INT(73)          \
    INT(74)          \
    INT(75)          \
    INT(76)          \
    INT(77)          \
    INT(78)          \
    INT(79)          \
    INT(80)          \
    INT(81)          \
    INT(82)          \
    INT(83)          \
    INT(84)          \
    INT(85)          \
    INT(86)          \
    INT(87)          \
    INT(88)          \
    INT(89)          \
    INT(90)          \
    INT(91)          \
    INT(92)          \
    INT(93)          \
    INT(94)          \
    INT(95)          \
    INT(96)          \
    INT(97)          \
    INT(98)          \
    INT(99)          \
    INT(100)         \
    INT(101)         \
    INT(102)         \
    INT(103)         \
    INT(104)         \
    INT(105)         \
    INT(106)         \
    INT(107)         \
    INT(108)         \
    INT(109)         \
    INT(110)         \
    INT(111)         \
    INT(112)         \
    INT(113)         \
    INT(114)         \
    INT(115)         \
    INT(116)         \
    INT(117)         \
    INT(118)         \
    INT(119)         \
    INT(120)         \
    INT(121)         \
    INT(122)         \
    INT(123)         \
    INT(124)         \
    INT(125)         \
    INT(126)         \
    INT(127)         \
    INT(128)         \
    INT(129)         \
    INT(130)         \
    INT(131)         \
    INT(132)         \
    INT(133)         \
    INT(134)         \
    INT(135)         \
    INT(136)         \
    INT(137)         \
    INT(138)         \
    INT(139)         \
    INT(140)         \
    INT(141)         \
    INT(142)         \
    INT(143)         \
    INT(144)         \
    INT(145)         \
    INT(146)         \
    INT(147)         \
    INT(148)         \
    INT(149)         \
    INT(150)         \
    INT(151)         \
    INT(152)         \
    INT(153)         \
    INT(154)         \
    INT(155)         \
    INT(156)         \
    INT(157)         \
    INT(158)         \
    INT(159)         \
    INT(160)         \
    INT(161)         \
    INT(162)         \
    INT(163)         \
    INT(164)         \
    INT(165)         \
    INT(166)         \
    INT(167)         \
    INT(168)         \
    INT(169)         \
    INT(170)         \
    INT(171)         \
    INT(172)         \
    INT(173)         \
    INT(174)         \
    INT(175)         \
    INT(176)         \
    INT(177)         \
    INT(178)         \
    INT(179)         \
    INT(180)         \
    INT(181)         \
    INT(182)         \
    INT(183)         \
    INT(184)         \
    INT(185)         \
    INT(186)         \
    INT(187)         \
    INT(188)         \
    INT(189)         \
    INT(190)         \
    INT(191)         \
    INT(192)         \
    INT(193)         \
    INT(194)         \
    INT(195)         \
    INT(196)         \
    INT(197)         \
    INT(198)         \
    INT(199)         \
    INT(200)         \
    INT(201)         \
    INT(202)         \
    INT(203)         \
    INT(204)         \
    INT(205)         \
    INT(206)         \
    INT(207)         \
    INT(208)         \
    INT(209)         \
    INT(210)         \
    INT(211)         \
    INT(212)         \
    INT(213)         \
    INT(214)         \
    INT(215)         \
    INT(216)         \
    INT(217)         \
    INT(218)         \
    INT(219)         \
    INT(220)         \
    INT(221)         \
    INT(222)         \
    INT(223)         \
    INT(224)         \
    INT(225)         \
    INT(226)         \
    INT(227)         \
    INT(228)         \
    INT(229)         \
    INT(230)         \
    INT(231)         \
    INT(232)         \
    INT(233)         \
    INT(234)         \
    INT(235)         \
    INT(236)         \
    INT(237)         \
    INT(238)         \
    INT(239)         \
    INT(240)         \
    INT(241)         \
    INT(242)         \
    INT(243)         \
    INT(244)         \
    INT(245)         \
    INT(246)         \
    INT(247)         \
    INT(248)         \
    INT(249)         \
    INT(250)         \
    INT(251)         \
    INT(252)         \
    INT(253)         \
    INT(254)         \
    INT(255)

IDT_TABLE

#undef INT
#define INT(x) INT_##x

#undef INT_RESERVED
#define INT_RESERVED(x) reinterpret_cast<FUNC>(0)

typedef void (*FUNC)();

static FUNC s_interruptTable[IDT::idtTableLen] = {
    INT(0), INT(1), INT(2), INT(3), INT(4), INT(5), INT(6), INT(7),
    INT(8), INT_RESERVED(9), INT(10), INT(11), INT(12), INT(13), INT(14), INT_RESERVED(15),
    INT(16), INT(17), INT(18), INT(19), INT(20), INT(21), INT_RESERVED(22), INT_RESERVED(23),
    INT_RESERVED(24), INT_RESERVED(25), INT_RESERVED(26), INT_RESERVED(27),
    INT_RESERVED(28), INT_RESERVED(29), INT_RESERVED(30), INT_RESERVED(31),
    INT(32), INT(33), INT(34), INT(35), INT(36), INT(37), INT(38), INT(39),
    INT(40), INT(41), INT(42), INT(43), INT(44), INT(45), INT(46), INT(47),
    INT(48), INT(49), INT(50), INT(51), INT(52), INT(53), INT(54), INT(55),
    INT(56), INT(57), INT(58), INT(59), INT(60), INT(61), INT(62), INT(63),
    INT(64), INT(65), INT(66), INT(67), INT(68), INT(69), INT(70), INT(71),
    INT(72), INT(73), INT(74), INT(75), INT(76), INT(77), INT(78), INT(79),
    INT(80), INT(81), INT(82), INT(83), INT(84), INT(85), INT(86), INT(87),
    INT(88), INT(89), INT(90), INT(91), INT(92), INT(93), INT(94), INT(95),
    INT(96), INT(97), INT(98), INT(99), INT(100), INT(101), INT(102), INT(103),
    INT(104), INT(105), INT(106), INT(107), INT(108), INT(109), INT(110), INT(111),
    INT(112), INT(113), INT(114), INT(115), INT(116), INT(117), INT(118), INT(119),
    INT(120), INT(121), INT(122), INT(123), INT(124), INT(125), INT(126), INT(127),
    INT(128), INT(129), INT(130), INT(131), INT(132), INT(133), INT(134), INT(135),
    INT(136), INT(137), INT(138), INT(139), INT(140), INT(141), INT(142), INT(143),
    INT(144), INT(145), INT(146), INT(147), INT(148), INT(149), INT(150), INT(151),
    INT(152), INT(153), INT(154), INT(155), INT(156), INT(157), INT(158), INT(159),
    INT(160), INT(161), INT(162), INT(163), INT(164), INT(165), INT(166), INT(167),
    INT(168), INT(169), INT(170), INT(171), INT(172), INT(173), INT(174), INT(175),
    INT(176), INT(177), INT(178), INT(179), INT(180), INT(181), INT(182), INT(183),
    INT(184), INT(185), INT(186), INT(187), INT(188), INT(189), INT(190), INT(191),
    INT(192), INT(193), INT(194), INT(195), INT(196), INT(197), INT(198), INT(199),
    INT(200), INT(201), INT(202), INT(203), INT(204), INT(205), INT(206), INT(207),
    INT(208), INT(209), INT(210), INT(211), INT(212), INT(213), INT(214), INT(215),
    INT(216), INT(217), INT(218), INT(219), INT(220), INT(221), INT(222), INT(223),
    INT(224), INT(225), INT(226), INT(227), INT(228), INT(229), INT(230), INT(231),
    INT(232), INT(233), INT(234), INT(235), INT(236), INT(237), INT(238), INT(239),
    INT(240), INT(241), INT(242), INT(243), INT(244), INT(245), INT(246), INT(247),
    INT(248), INT(249), INT(250), INT(251), INT(252), INT(253), INT(254), INT(255)
};

/**
 * @brief Array holding the IDT entries
 *
 */
static IDTEntry s_idt[idtTableLen];

/**
 * @brief IDTR
 *
 */
static IDTR s_idtr;

static void setEntry(int index, void* handler, uint16_t selector, uint8_t typeAttr);

/**
 * @brief Initializes the IDT.
 *
 */
void init()
{
    Handlers::init();

    int i = 0;

    for (; i < 32; i++) {
        setEntry(i, (void*)s_interruptTable[i],
            (uint16_t)SegmentSelect::KERNELCODE,
            IDTEntry::PRESENT | IDTEntry::RING0 | IDTEntry::GateTrap);
    }

    for (; i < IDT::idtTableLen; i++) {
        setEntry(i, (void*)s_interruptTable[i],
            (uint16_t)SegmentSelect::KERNELCODE,
            IDTEntry::PRESENT | IDTEntry::RING0 | IDTEntry::GateInt);
    }

    // Set the IDTR structure
    s_idtr.limit = sizeof(s_idt) - 1;
    s_idtr.base = reinterpret_cast<uintptr_t>(&s_idt);

    // Load the IDT using the `lidt` assembly instruction
    __asm__ volatile("lidt %0"
                     :
                     : "m"(s_idtr));
}

/**
 * @brief Sets an entry in the IDT.
 *
 * @param index The index of the IDT entry to set.
 * @param handler The address of the interrupt handler.
 * @param selector The code segment selector.
 * @param typeAttr The type and attributes of the entry.
 *
 * Configures an entry in the IDT with the provided handler address, code segment
 * selector, and type attributes.
 */
static void setEntry(int index, void* handler, uint16_t selector, uint8_t typeAttr)
{
    // Configure the IDT entry
    s_idt[index].isrLow = (uintptr_t)handler & 0xFFFF;
    s_idt[index].kernelCs = selector;
    s_idt[index].ist = 0 & 0x7;
    s_idt[index].attributes = typeAttr;
    s_idt[index].isrMid = ((uintptr_t)handler >> 16) & 0xFFFF;
    s_idt[index].isrHigh = ((uintptr_t)handler >> 32) & 0xFFFFFFFF;
    s_idt[index].reserved = 0;
}

/**
 * @brief Exception handler
 * @param regs The status of registers at exception time
 */
extern "C" void intHandler(const InterruptFrame& regs)
{
    const char* intStr;

    switch (static_cast<ExternalInterrupts>(regs.int_no)) {
    case ExternalInterrupts::DivideError:
        intStr = "DivideError";
        break;

    case ExternalInterrupts::DebugException:
        intStr = "DebugException";
        break;

    case ExternalInterrupts::NMIInterrupt:
        intStr = "NMIInterrupt";
        break;

    case ExternalInterrupts::Breakpoint:
        intStr = "Breakpoint";
        break;

    case ExternalInterrupts::Overflow:
        intStr = "Overflow";
        break;

    case ExternalInterrupts::BOUNDRangeExceeded:
        intStr = "BOUNDRangeExceeded";
        break;

    case ExternalInterrupts::InvalidOpcode:
        intStr = "InvalidOpcode";
        break;

    case ExternalInterrupts::DeviceNotAvailable:
        intStr = "DeviceNotAvailable";
        break;

    case ExternalInterrupts::DoubleFault:
        intStr = "DoubleFault";
        break;

    case ExternalInterrupts::CoprocessorSegmentOverrun:
        intStr = "CoprocessorSegmentOverrun";
        break;

    case ExternalInterrupts::Reserved:
        intStr = "reserved";
        break;

    case ExternalInterrupts::InvalidTSS:
        intStr = "InvalidTSS";
        break;

    case ExternalInterrupts::SegmentNotPresent:
        intStr = "SegmentNotPresent";
        break;

    case ExternalInterrupts::StackSegmentFault:
        intStr = "StackSegmentFault";
        break;

    case ExternalInterrupts::GeneralProtection:
        intStr = "GeneralProtection";
        break;

    case ExternalInterrupts::PageFault:
        intStr = "PageFault";
        break;

    case ExternalInterrupts::IntelReserved:
        intStr = "IntelReserved";
        break;

    case ExternalInterrupts::X87FpuFloatingPointError:
        intStr = "x87_FPU_FloatingPointError";
        break;

    case ExternalInterrupts::AlignmentCheck:
        intStr = "AlignmentCheck";
        break;

    case ExternalInterrupts::MachineCheck:
        intStr = "MachineCheck";
        break;

    case ExternalInterrupts::SimdFloatingPointException:
        intStr = "SIMD_FloatingPointException";
        break;

    case ExternalInterrupts::VirtualizationException:
        intStr = "VirtualizationException";
        break;

    case ExternalInterrupts::ControlProtectionException:
        intStr = "ControlProtectionException";
        break;

    default:
        intStr = "OtherException";
        Handlers::interruptHandler(regs.int_no);
        return;
    }

    printf("==== EXCEPTION/INTERRUPT INFO ====\n");
    printf("INT_NO  = 0x%lx %s\n", regs.int_no, intStr);
    printf("ERR_CODE= 0x%lx\n", regs.err_code);

    printf("\n--- General Purpose Registers ---\n");
    printf("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx\n",
        regs.rax, regs.rbx, regs.rcx, regs.rdx);
    printf("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx\n",
        regs.rsi, regs.rdi, regs.rbp);
    printf("R8 =0x%016lx R9 =0x%016lx R10=0x%016lx R11=0x%016lx\n",
        regs.r8, regs.r9, regs.r10, regs.r11);
    printf("R12=0x%016lx R13=0x%016lx R14=0x%016lx R15=0x%016lx\n",
        regs.r12, regs.r13, regs.r14, regs.r15);

    printf("\n--- CPU State ---\n");
    printf("RIP=0x%016lx CS=0x%04lx RFLAGS=0x%016lx\n",
        regs.rip, regs.cs, regs.rflags);
    printf("RSP=0x%016lx SS=0x%04lx\n", regs.rsp, regs.ss);
    printf("=================================\n");

    while (1)
        ;
}
}; // namespace IDT