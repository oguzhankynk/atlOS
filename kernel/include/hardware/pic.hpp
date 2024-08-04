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
    /**
     * @brief Initializes the PIC8259.
     *
     * This function initializes the master and slave PICs with the desired settings.
     * It performs the initial configuration of the PICs for use in the system.
     */
    void Init();

    /**
     * @brief Sends an end-of-interrupt (EOI) signal to the master PIC.
     *
     * @param irq Interrupt request number (IRQ) that has been serviced.
     */
    void send_master_eoi(uint8_t irq);

    /**
     * @brief Sends an end-of-interrupt (EOI) signal to the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) that has been serviced.
     */
    void send_slave_eoi(uint8_t irq);

    /**
     * @brief Masks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_master_irq(uint8_t irq);

    /**
     * @brief Masks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_slave_irq(uint8_t irq);

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_master_irq(uint8_t irq);

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_slave_irq(uint8_t irq);

    /**
     * @brief Disables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to disable.
     */
    void irq_disable(uint8_t irq);

    /**
     * @brief Enables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to enable.
     */
    void irq_enable(uint8_t irq);

}; // namespace PIC

#endif /* AD370CF8_142A_4BD1_845C_6DBAF45AC5BC */
