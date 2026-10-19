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

#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <drivers/pic.hpp>
#include <stdio.h>

namespace PIC {

// I/O port addresses for the master and slave PICs
static constexpr uint16_t masterCommandPort = 0x20;
static constexpr uint16_t masterDataPort = 0x21;
static constexpr uint16_t slaveCommandPort = 0xA0;
static constexpr uint16_t slaveDataPort = 0xA1;

/**
 * @brief Sends a command to the master PIC.
 *
 * @param command
 */
static void sendMasterCommand(uint8_t command)
{
    IOPort::outportb(masterCommandPort, command);
}

/**
 * @brief Sends a command to the slave PIC.
 *
 * @param command
 */
static void sendSlaveCommand(uint8_t command)
{
    IOPort::outportb(slaveCommandPort, command);
}

/**
 * @brief Writes a value to the master PIC data port.
 *
 * @param data
 */
static void writeMasterData(uint8_t data)
{
    IOPort::outportb(masterDataPort, data);
}

/**
 * @brief Writes a value to the slave PIC data port.
 *
 * @param data
 */
static void writeSlaveData(uint8_t data)
{
    IOPort::outportb(slaveDataPort, data);
}

/**
 * @brief Initializes the PIC8259.
 *
 * This function initializes the master and slave PICs with the desired settings.
 * It performs the initial configuration of the PICs for use in the system.
 */
void init()
{
    // IOPort::disableInterrupts();

    // Initialization command words (ICW) for master and slave PICs
    uint8_t masterIcw1 = 0x11; // ICW1: Start initialization, edge triggered, cascade mode
    uint8_t slaveIcw1 = 0x11; // ICW1: Start initialization, edge triggered, cascade mode

    // ICW2: Base interrupt vector offset for master and slave
    uint8_t masterIcw2 = 0x20; // Base interrupt vector offset for master (0x20)
    uint8_t slaveIcw2 = 0x28; // Base interrupt vector offset for slave (0x28)

    // ICW3: Master to slave connection details
    uint8_t masterIcw3 = 0x04; // Master PIC connected to slave at IRQ2
    uint8_t slaveIcw3 = 0x02; // Slave PIC connected to master at IRQ2

    // ICW4: Normal EOI, 8086/88 mode
    uint8_t icw4 = 0x01; // Normal EOI, 8086 mode

    // Send initialization command words to master and slave PICs
    IOPort::outportb(masterCommandPort, masterIcw1);
    IOPort::outportb(slaveCommandPort, slaveIcw1);

    IOPort::outportb(masterDataPort, masterIcw2);
    IOPort::outportb(slaveDataPort, slaveIcw2);

    IOPort::outportb(masterDataPort, masterIcw3);
    IOPort::outportb(slaveDataPort, slaveIcw3);

    IOPort::outportb(masterDataPort, icw4);
    IOPort::outportb(slaveDataPort, icw4);

    // Mask all interrupts on master and slave PICs initially
    IOPort::outportb(masterDataPort, 0xFF); // Mask all interrupts
    IOPort::outportb(slaveDataPort, 0xFF); // Mask all interrupts

    IOPort::outportb(masterDataPort, ~(0x2)); // Mask all interrupts
    IOPort::outportb(slaveDataPort, ~(0x2)); // Mask all interrupts

    IOPort::enableInterrupts();
}

/**
 * @brief Sends an end-of-interrupt (EOI) signal to the PIC.
 *
 * @param irq Interrupt request number (IRQ) that has been serviced.
 */
void sendEoi(IRQ irq)
{

    if (static_cast<uint8_t>(irq) >= 8) {
        // Send EOI to master PIC
        IOPort::outportb(slaveCommandPort, 0x20);
    }

    // Send EOI to slave PIC
    IOPort::outportb(masterCommandPort, 0x20);
}

/**
 * @brief Masks a specific interrupt request (IRQ) on the master PIC.
 *
 * @param irq Interrupt request number (IRQ) to mask.
 */
void maskMasterIrq(IRQ irq)
{
    // Read current mask
    uint8_t mask = IOPort::inportb(masterDataPort);
    // Set the bit for the specified IRQ to mask it
    mask |= (1 << static_cast<uint8_t>(irq));
    // Write updated mask
    IOPort::outportb(masterDataPort, mask);
}

/**
 * @brief Masks a specific interrupt request (IRQ) on the slave PIC.
 *
 * @param irq Interrupt request number (IRQ) to mask.
 */
void maskSlaveIrq(IRQ irq)
{
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(slaveDataPort);
    // Mask the specified IRQ by setting the appropriate bit
    mask |= (1 << static_cast<uint8_t>(irq));
    // Write the updated mask back to the slave PIC
    IOPort::outportb(slaveDataPort, mask);
}

/**
 * @brief Unmasks a specific interrupt request (IRQ) on the master PIC.
 *
 * @param irq Interrupt request number (IRQ) to unmask.
 */
void unmaskMasterIrq(IRQ irq)
{
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(masterDataPort);
    // Unmask the specified IRQ by clearing the appropriate bit
    mask &= ~(1 << static_cast<uint8_t>(irq));
    // Write the updated mask back to the master PIC
    IOPort::outportb(masterDataPort, mask);
}

/**
 * @brief Unmasks a specific interrupt request (IRQ) on the slave PIC.
 *
 * @param irq Interrupt request number (IRQ) to unmask.
 */
void unmaskSlaveIrq(IRQ irq)
{
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(slaveDataPort);
    // Unmask the specified IRQ by clearing the appropriate bit
    mask &= ~(1 << static_cast<uint8_t>(irq));
    // Write the updated mask back to the slave PIC
    IOPort::outportb(slaveDataPort, mask);
}

/**
 * @brief Disables a specific interrupt request (IRQ).
 *
 * @param irq Interrupt request number (IRQ) to disable.
 */
void irqDisable(IRQ irq)
{
    if (static_cast<uint8_t>(irq) < 8) {
        // IRQ is in the range of the master PIC
        uint8_t mask = IOPort::inportb(masterDataPort);
        mask |= (1 << static_cast<uint8_t>(irq)); // Set the bit to mask the IRQ
        IOPort::outportb(masterDataPort, mask);
    } else {
        // IRQ is in the range of the slave PIC
        uint8_t mask = IOPort::inportb(slaveDataPort);
        mask |= (1 << (static_cast<uint8_t>(irq) - 8)); // Set the bit to mask the IRQ
        IOPort::outportb(slaveDataPort, mask);
    }
}

/**
 * @brief Enables a specific interrupt request (IRQ).
 *
 * @param irq Interrupt request number (IRQ) to enable.
 */
void irqEnable(IRQ irq)
{
    if (static_cast<uint8_t>(irq) < 8) {
        // IRQ is in the range of the master PIC
        uint8_t mask = IOPort::inportb(masterDataPort);
        mask &= ~(1 << static_cast<uint8_t>(irq)); // Clear the bit to unmask the IRQ
        IOPort::outportb(masterDataPort, mask);
    } else {
        // IRQ is in the range of the slave PIC
        uint8_t mask = IOPort::inportb(slaveDataPort);
        mask &= ~(1 << (static_cast<uint8_t>(irq) - 8)); // Clear the bit to unmask the IRQ
        IOPort::outportb(slaveDataPort, mask);
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
bool addIrqHandler(IRQ irq, void (*handler)(void*), void* arg)
{
    return Handlers::addInterruptHandler(static_cast<uint32_t>(irq) + 32, handler, arg);
}
}; // namespace PIC
