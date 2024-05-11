/**
 * @file pic8259.cpp
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

#include <hardware/pic8259.hpp>
#include <kernel/sys/IOPort.hpp>

// Constructor
PIC8259::PIC8259() {
    // Default constructor; can be used to set initial states if necessary.
}

// Initializes the PIC8259.
void PIC8259::init() {
    asm("cli");
    // Initialization command words (ICW) for master and slave PICs
    uint8_t master_icw1 = 0x11; // ICW1: Start initialization, edge triggered, cascade mode
    uint8_t slave_icw1 = 0x11; // ICW1: Start initialization, edge triggered, cascade mode
    
    // ICW2: Base interrupt vector offset for master and slave
    uint8_t master_icw2 = 0x20; // Base interrupt vector offset for master (0x20)
    uint8_t slave_icw2 = 0x28; // Base interrupt vector offset for slave (0x28)
    
    // ICW3: Master to slave connection details
    uint8_t master_icw3 = 0x04; // Master PIC connected to slave at IRQ2
    uint8_t slave_icw3 = 0x02; // Slave PIC connected to master at IRQ2
    
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
    IOPort::outportb(SLAVE_DATA_PORT, 0xFF); // Mask all interrupts

    asm("sti");     
}

// Sends an end-of-interrupt (EOI) signal to the master PIC.
void PIC8259::send_master_eoi(uint8_t irq) {
    // Send EOI to master PIC
    IOPort::outportb(MASTER_COMMAND_PORT, 0x20);
}

// Sends an end-of-interrupt (EOI) signal to the slave PIC.
void PIC8259::send_slave_eoi(uint8_t irq) {
    // Send EOI to slave PIC
    IOPort::outportb(SLAVE_COMMAND_PORT, 0x20);
}

// Masks a specific interrupt request (IRQ) on the master PIC.
void PIC8259::mask_master_irq(uint8_t irq) {
    // Read current mask
    uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
    // Set the bit for the specified IRQ to mask it
    mask |= (1 << irq);
    // Write updated mask
    IOPort::outportb(MASTER_DATA_PORT, mask);
}


// Masks a specific interrupt request (IRQ) on the slave PIC.
void PIC8259::mask_slave_irq(uint8_t irq) {
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
    // Mask the specified IRQ by setting the appropriate bit
    mask |= (1 << irq);
    // Write the updated mask back to the slave PIC
    IOPort::outportb(SLAVE_DATA_PORT, mask);
}

// Unmasks a specific interrupt request (IRQ) on the master PIC.
void PIC8259::unmask_master_irq(uint8_t irq) {
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
    // Unmask the specified IRQ by clearing the appropriate bit
    mask &= ~(1 << irq);
    // Write the updated mask back to the master PIC
    IOPort::outportb(MASTER_DATA_PORT, mask);
}

// Unmasks a specific interrupt request (IRQ) on the slave PIC.
void PIC8259::unmask_slave_irq(uint8_t irq) {
    // Read the current interrupt mask
    uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
    // Unmask the specified IRQ by clearing the appropriate bit
    mask &= ~(1 << irq);
    // Write the updated mask back to the slave PIC
    IOPort::outportb(SLAVE_DATA_PORT, mask);
}

// Disables a specific interrupt request (IRQ).
void PIC8259::irq_disable(uint8_t irq) {
    if (irq < 8) {
        // IRQ is in the range of the master PIC
        uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
        mask |= (1 << irq); // Set the bit to mask the IRQ
        IOPort::outportb(MASTER_DATA_PORT, mask);
    } else {
        // IRQ is in the range of the slave PIC
        irq -= 8; // Adjust IRQ number for the slave PIC
        uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
        mask |= (1 << irq); // Set the bit to mask the IRQ
        IOPort::outportb(SLAVE_DATA_PORT, mask);
    }
}

// Enables a specific interrupt request (IRQ).
void PIC8259::irq_enable(uint8_t irq) {
    if (irq < 8) {
        // IRQ is in the range of the master PIC
        uint8_t mask = IOPort::inportb(MASTER_DATA_PORT);
        mask &= ~(1 << irq); // Clear the bit to unmask the IRQ
        IOPort::outportb(MASTER_DATA_PORT, mask);
    } else {
        // IRQ is in the range of the slave PIC
        irq -= 8; // Adjust IRQ number for the slave PIC
        uint8_t mask = IOPort::inportb(SLAVE_DATA_PORT);
        mask &= ~(1 << irq); // Clear the bit to unmask the IRQ
        IOPort::outportb(SLAVE_DATA_PORT, mask);
    }
}

// Sends a command to the master PIC.
void PIC8259::send_master_command(uint8_t command) {
    IOPort::outportb(MASTER_COMMAND_PORT, command);
}

// Sends a command to the slave PIC.
void PIC8259::send_slave_command(uint8_t command) {
    IOPort::outportb(SLAVE_COMMAND_PORT, command);
}

// Writes a value to the master PIC data port.
void PIC8259::write_master_data(uint8_t data) {
    IOPort::outportb(MASTER_DATA_PORT, data);
}

// Writes a value to the slave PIC data port.
void PIC8259::write_slave_data(uint8_t data) {
    IOPort::outportb(SLAVE_DATA_PORT, data);
}