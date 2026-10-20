/**
 * @file keyboard.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::PS2Keyboard: characters typed on a PS/2 keyboard (IRQ 1).
 */

#include <drivers/keyboard.hpp>

#include <arch/x86_64/io/ioport.hpp>

namespace Drivers {

// 8042 PS/2 controller
static constexpr uint16_t dataPort = 0x60;
static constexpr uint16_t statusPort = 0x64;
static constexpr uint8_t outputFull = 1 << 0; // status: a byte is waiting at dataPort

PS2Keyboard::PS2Keyboard(InterruptController& controller, CharacterQueue& queue)
    : m_queue(queue)
{
    // Bytes left over from the firmware would otherwise keep IRQ 1 from firing again
    while (IOPort::inportb(statusPort) & outputFull)
        IOPort::inportb(dataPort);

    controller.attach(Irq::Keyboard, onInterrupt, this);
    controller.unmask(Irq::Keyboard);
}

void PS2Keyboard::onInterrupt(void* keyboard)
{
    static_cast<PS2Keyboard*>(keyboard)->handleScancode(IOPort::inportb(dataPort));
}

// Extended make code of an arrow key -> the last byte of its ANSI sequence
static char arrowFinalByte(uint8_t code)
{
    switch (code) {
    case 0x48:
        return 'A'; // up
    case 0x50:
        return 'B'; // down
    case 0x4D:
        return 'C'; // right
    case 0x4B:
        return 'D'; // left
    default:
        return 0;
    }
}

void PS2Keyboard::handleScancode(uint8_t byte)
{
    KeyEvent event;
    if (!m_decoder.feed(byte, event) || !event.pressed)
        return;

    if (event.character) {
        m_queue.push(event.character);
        return;
    }

    char arrow = event.extended ? arrowFinalByte(event.code) : static_cast<char>(0);
    if (arrow) {
        const char sequence[3] = { '\x1b', '[', arrow };
        m_queue.push(sequence, sizeof(sequence));
    }
}

} // namespace Drivers
