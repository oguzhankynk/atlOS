/**
 * @file keyboard.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::PS2Keyboard: characters typed on a PS/2 keyboard (IRQ 1).
 */

#include <drivers/keyboard.hpp>

#include <arch/x86_64/io/ioport.hpp>
#include <sync/interrupt_guard.hpp>

namespace Drivers {

// 8042 PS/2 controller
static constexpr uint16_t dataPort = 0x60;
static constexpr uint16_t statusPort = 0x64;
static constexpr uint8_t outputFull = 1 << 0; // status: a byte is waiting at dataPort

PS2Keyboard::PS2Keyboard(InterruptController& controller, Threading::Scheduler& scheduler)
    : m_available(scheduler, 0, bufferSize)
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

void PS2Keyboard::handleScancode(uint8_t byte)
{
    KeyEvent event;
    if (!m_decoder.feed(byte, event) || !event.character)
        return;

    // Also called by threads in the self tests: the buffer is shared with the IRQ
    Sync::InterruptGuard guard;

    if (m_count == bufferSize) {
        m_dropped++;
        return;
    }

    m_buffer[(m_head + m_count) % bufferSize] = event.character;
    m_count++;
    m_available.give();
}

char PS2Keyboard::pop()
{
    Sync::InterruptGuard guard;

    char c = m_buffer[m_head];
    m_head = (m_head + 1) % bufferSize;
    m_count--;
    return c;
}

char PS2Keyboard::read()
{
    // A unit means a character is in the buffer and nobody else will pop it
    m_available.take();
    return pop();
}

bool PS2Keyboard::tryRead(char& c)
{
    if (!m_available.tryTake())
        return false;
    c = pop();
    return true;
}

} // namespace Drivers
