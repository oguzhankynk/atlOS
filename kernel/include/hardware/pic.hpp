/**
 * @file pic.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Class for interfacing with the 8259 Programmable Interrupt Controller (PIC).
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
#ifndef AD370CF8_142A_4BD1_845C_6DBAF45AC5BC
#define AD370CF8_142A_4BD1_845C_6DBAF45AC5BC

#include <stdint.h>

/**
 * @brief Namespace for interfacing with the 8259 Programmable Interrupt Controller (PIC).
 */
namespace PIC
{
    // IRQ  (x86 - 8259 PIC)
    enum class IRQ : uint8_t
    {
        Timer = 0,        // IRQ0 - Sistem Timer (PIT)
        Keyboard = 1,     // IRQ1 - Keyboard
        Cascade = 2,      // IRQ2 - PIC Cascade (Slave to Master)
        COM2 = 3,         // IRQ3 - COM2 (Serial Port 2)
        COM1 = 4,         // IRQ4 - COM1 (Serial Port 1)
        LPT2 = 5,         // IRQ5 - LPT2 (Parallel Port 2)
        Floppy = 6,       // IRQ6 - Floppy Disk Controller
        LPT1 = 7,         // IRQ7 - LPT1 (Parallel Port 1)
        RTC = 8,          // IRQ8 - CMOS Real-Time Clock (RTC)
        Redirected = 9,   // IRQ9 - Free (Redirected to IRQ2)
        Free10 = 10,      // IRQ10 - Free
        Free11 = 11,      // IRQ11 - Free
        PS2Mouse = 12,    // IRQ12 - PS/2 Mouse
        FPU = 13,         // IRQ13 - FPU (Floating Point Unit) / Co-Processor
        PrimaryATA = 14,  // IRQ14 - Primary ATA / IDE Controller
        SecondaryATA = 15 // IRQ15 - Secondary ATA / IDE Controller
    };

    /**
     * @brief Initializes the PIC8259.
     *
     * This function initializes the master and slave PICs with the desired settings.
     * It performs the initial configuration of the PICs for use in the system.
     */
    void Init();

    /**
     * @brief Sends an end-of-interrupt (EOI) signal to the PIC.
     *
     * @param irq Interrupt request number (IRQ) that has been serviced.
     */
    void send_eoi(IRQ irq);

    /**
     * @brief Masks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_master_irq(IRQ irq);

    /**
     * @brief Masks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_slave_irq(IRQ irq);

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_master_irq(IRQ irq);

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_slave_irq(IRQ irq);

    /**
     * @brief Disables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to disable.
     */
    void irq_disable(IRQ irq);

    /**
     * @brief Enables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to enable.
     */
    void irq_enable(IRQ irq);

    /**
     * @brief Adds an interrupt request (IRQ) handler.
     *
     * @param irq Interrupt request number (IRQ) to add the handler for.
     * @param handler Function pointer to the handler function.
     * @param arg Argument to be passed to the handler function.
     *
     * @return true if the handler was successfully added, false otherwise.
     */
    bool add_irq_handler(IRQ irq, void (*handler)(void *), void *arg = nullptr);

}; // namespace PIC

#endif /* AD370CF8_142A_4BD1_845C_6DBAF45AC5BC */
