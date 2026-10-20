/**
 * @file keyboard.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::PS2Keyboard: characters typed on a PS/2 keyboard (IRQ 1).
 */

#pragma once

#include <stdint.h>

#include <drivers/character_queue.hpp>
#include <drivers/interrupt_controller.hpp>
#include <drivers/scancode.hpp>

namespace Drivers {

/**
 * @brief The keyboard behind the 8042 PS/2 controller.
 *
 * The IRQ handler reads one scancode byte, decodes it and pushes the
 * character, if any, into a CharacterQueue. The arrow keys become the ANSI
 * escape sequences a serial terminal sends ("\x1b[A" for up), so a reader
 * handles both inputs the same way.
 */
class PS2Keyboard {
public:
    /// Empties the controller's output buffer, attaches to Irq::Keyboard and unmasks it.
    PS2Keyboard(InterruptController& controller, CharacterQueue& queue);

    PS2Keyboard(const PS2Keyboard&) = delete;
    PS2Keyboard& operator=(const PS2Keyboard&) = delete;

    /// What the IRQ handler does with a byte from the controller (public for the self tests).
    void handleScancode(uint8_t byte);

    const ScancodeDecoder& decoder() const { return m_decoder; }

private:
    static void onInterrupt(void* keyboard);

    ScancodeDecoder m_decoder;
    CharacterQueue& m_queue;
};

} // namespace Drivers
