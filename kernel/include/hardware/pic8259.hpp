/**
 * @file pic8259.hpp
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

#include <common/Singleton.hpp>
#include <stdint.h>

/**
 * @class PIC8259
 * @brief Class for interfacing with the 8259 Programmable Interrupt Controller (PIC).
 */
class PIC8259 : public Singleton<PIC8259>{
public:
    // I/O port addresses for the master and slave PICs
    static const uint16_t MASTER_COMMAND_PORT = 0x20;
    static const uint16_t MASTER_DATA_PORT = 0x21;
    static const uint16_t SLAVE_COMMAND_PORT = 0xA0;
    static const uint16_t SLAVE_DATA_PORT = 0xA1;

    /**
     * @brief Constructor.
     */
    PIC8259();

    /**
     * @brief Initializes the PIC8259.
     *
     * This function initializes the master and slave PICs with the desired settings.
     * It performs the initial configuration of the PICs for use in the system.
     */
    void init();

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

private:
    /**
     * @brief Sends a command to the master PIC.
     *
     * @param command The command to send.
     */
    void send_master_command(uint8_t command);

    /**
     * @brief Sends a command to the slave PIC.
     *
     * @param command The command to send.
     */
    void send_slave_command(uint8_t command);

    /**
     * @brief Writes a value to the master PIC data port.
     *
     * @param data The data value to write.
     */
    void write_master_data(uint8_t data);

    /**
     * @brief Writes a value to the slave PIC data port.
     *
     * @param data The data value to write.
     */
    void write_slave_data(uint8_t data);
};



#endif /* AD370CF8_142A_4BD1_845C_6DBAF45AC5BC */
