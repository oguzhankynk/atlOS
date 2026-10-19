/**
 * @file pic.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Implemantation for interfacing with the 8259 Programmable Interrupt Controller (PIC).
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

#include <drivers/pic.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <arch/x86_64/idt/handlers.hpp>
#include <stdio.h>

namespace PIC
{

    // I/O port addresses for the master and slave PICs
    static constexpr uint16_t MASTER_COMMAND_PORT = 0x20;
    static constexpr uint16_t MASTER_DATA_PORT = 0x21;
    static constexpr uint16_t SLAVE_COMMAND_PORT = 0xA0;
    static constexpr uint16_t SLAVE_DATA_PORT = 0xA1;

    /**
     * @brief Sends a command to the master PIC.
     *
     * @param command
     */
    static void send_master_command(uint8_t command)
    {
        IOPort::outportb(MASTER_COMMAND_PORT, command);
    }

    /**
     * @brief Sends a command to the slave PIC.
     *
     * @param command
     */
    static void send_slave_command(uint8_t command)
    {
        IOPort::outportb(SLAVE_COMMAND_PORT, command);
    }

    /**
     * @brief Writes a value to the master PIC data port.
     *
     * @param data
     */
    static void write_master_data(uint8_t data)
    {
        IOPort::outportb(MASTER_DATA_PORT, data);
    }

    /**
     * @brief Writes a value to the slave PIC data port.
     *
     * @param data
     */
    static void write_slave_data(uint8_t data)
    {
        IOPort::outportb(SLAVE_DATA_PORT, data);
    }

    /**
     * @brief Initializes the PIC8259.
     *
     * This function initializes the master and slave PICs with the desired settings.
     * It performs the initial configuration of the PICs for use in the system.
     */
    void Init()
    {
        // IOPort::DisableInterrupts();

        // Initialization command words (ICW) for master and slave PICs
        uint8_t master_icw1 = 0x11; // ICW1: Start initialization, edge triggered, cascade mode
        uint8_t slave_icw1 = 0x11;  // ICW1: Start initialization, edge triggered, cascade mode

        // ICW2: Base interrupt vector offset for master and slave
        uint8_t master_icw2 = 0x20; // Base interrupt vector offset for master (0x20)
        uint8_t slave_icw2 = 0x28;  // Base interrupt vector offset for slave (0x28)

        // ICW3: Master to slave connection details
        uint8_t master_icw3 = 0x04; // Master PIC connected to slave at IRQ2
        uint8_t slave_icw3 = 0x02;  // Slave PIC connected to master at IRQ2

        // ICW4: Normal EOI, 8086/88 mode
        uint8_t icw4 = 0x01; // Normal EOI, 8086 mode

        // Send initialization command words to master and slave PICs
        IOPort::outportb(MASTER_COMMAND_PORT, master_icw1);
        IOPort::outportb(SLAVE_COMMAND_PORT, slave_icw1);

        IOPort::outportb(MASTER_DATA_PORT, master_icw2);
        IOPort::outportb(SLAVE_DATA_PORT, slave_icw2);

        IOPort::outportb(MASTER_DATA_PORT, master_icw3);
        IOPort::outportb(SLAVE_DATA_PORT, slave_icw3);

        IOPort::outportb(MASTER_DATA_PORT, icw4);
        IOPort::outportb(SLAVE_DATA_PORT, icw4);

        // Mask all interrupts on master and slave PICs initially
        IOPort::outportb(MASTER_DATA_PORT, 0xFF); // Mask all interrupts
        IOPort::outportb(SLAVE_DATA_PORT, 0xFF);  // Mask all interrupts

        IOPort::outportb(MASTER_DATA_PORT, ~(0x2)); // Mask all interrupts
        IOPort::outportb(SLAVE_DATA_PORT, ~(0x2));  // Mask all interrupts

        IOPort::EnableInterrupts();
    }

    /**
     * @brief Sends an end-of-interrupt (EOI) signal to the PIC.
     *
     * @param irq Interrupt request number (IRQ) that has been serviced.
     */
    void send_eoi(IRQ irq)
    {

        if (static_cast<uint8_t>(irq) >= 8)
        {
            // Send EOI to master PIC
            IOPort::outportb(SLAVE_COMMAND_PORT, 0x20);
        }

        // Send EOI to slave PIC
        IOPort::outportb(MASTER_COMMAND_PORT, 0x20);
    }

    /**
     * @brief Masks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_master_irq(IRQ irq)
    {
        // Read current mask
        uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
        // Set the bit for the specified IRQ to mask it
        mask |= (1 << static_cast<uint8_t>(irq));
        // Write updated mask
        IOPort::outportb(MASTER_DATA_PORT, mask);
    }

    /**
     * @brief Masks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to mask.
     */
    void mask_slave_irq(IRQ irq)
    {
        // Read the current interrupt mask
        uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
        // Mask the specified IRQ by setting the appropriate bit
        mask |= (1 << static_cast<uint8_t>(irq));
        // Write the updated mask back to the slave PIC
        IOPort::outportb(SLAVE_DATA_PORT, mask);
    }

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the master PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_master_irq(IRQ irq)
    {
        // Read the current interrupt mask
        uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
        // Unmask the specified IRQ by clearing the appropriate bit
        mask &= ~(1 << static_cast<uint8_t>(irq));
        // Write the updated mask back to the master PIC
        IOPort::outportb(MASTER_DATA_PORT, mask);
    }

    /**
     * @brief Unmasks a specific interrupt request (IRQ) on the slave PIC.
     *
     * @param irq Interrupt request number (IRQ) to unmask.
     */
    void unmask_slave_irq(IRQ irq)
    {
        // Read the current interrupt mask
        uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
        // Unmask the specified IRQ by clearing the appropriate bit
        mask &= ~(1 << static_cast<uint8_t>(irq));
        // Write the updated mask back to the slave PIC
        IOPort::outportb(SLAVE_DATA_PORT, mask);
    }

    /**
     * @brief Disables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to disable.
     */
    void irq_disable(IRQ irq)
    {
        if (static_cast<uint8_t>(irq) < 8)
        {
            // IRQ is in the range of the master PIC
            uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
            mask |= (1 << static_cast<uint8_t>(irq)); // Set the bit to mask the IRQ
            IOPort::outportb(MASTER_DATA_PORT, mask);
        }
        else
        {
            // IRQ is in the range of the slave PIC
            uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
            mask |= (1 << (static_cast<uint8_t>(irq) - 8)); // Set the bit to mask the IRQ
            IOPort::outportb(SLAVE_DATA_PORT, mask);
        }
    }

    /**
     * @brief Enables a specific interrupt request (IRQ).
     *
     * @param irq Interrupt request number (IRQ) to enable.
     */
    void irq_enable(IRQ irq)
    {
        if (static_cast<uint8_t>(irq) < 8)
        {
            // IRQ is in the range of the master PIC
            uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
            mask &= ~(1 << static_cast<uint8_t>(irq)); // Clear the bit to unmask the IRQ
            IOPort::outportb(MASTER_DATA_PORT, mask);
        }
        else
        {
            // IRQ is in the range of the slave PIC
            uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
            mask &= ~(1 << (static_cast<uint8_t>(irq) - 8)); // Clear the bit to unmask the IRQ
            IOPort::outportb(SLAVE_DATA_PORT, mask);
        }
    }

    /**
     * @brief Adds an interrupt request (IRQ) handler.
     *
     * @param irq Interrupt request number (IRQ) to add the handler for.
     * @param handler Function pointer to the handler function.
     * @param arg Argument to be passed to the handler function.
     *
     * @return true if the handler was successfully added, false otherwise.
     */
    bool add_irq_handler(IRQ irq, void (*handler)(void *), void *arg)
    {
        return Handlers::AddInterruptHandler(static_cast<uint32_t>(irq) + 32, handler, arg);
    }
}; // namespace PIC
