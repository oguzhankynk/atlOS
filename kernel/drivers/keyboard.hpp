/**
 * @file keyboard.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::PS2Keyboard: characters typed on a PS/2 keyboard (IRQ 1).
 */

#pragma once

#include <stdint.h>

#include <drivers/character_source.hpp>
#include <drivers/interrupt_controller.hpp>
#include <drivers/scancode.hpp>
#include <sync/semaphore.hpp>

namespace Drivers {

/**
 * @brief The keyboard behind the 8042 PS/2 controller.
 *
 * The IRQ handler reads one scancode byte, decodes it and puts the character,
 * if any, into a buffer; read() waits on a semaphore that counts the buffered
 * characters. When the buffer is full new characters are dropped (the ones
 * already typed keep their order).
 */
class PS2Keyboard final : public CharacterSource {
public:
    static constexpr uint32_t bufferSize = 64;

    /// Empties the controller's output buffer, attaches to Irq::Keyboard and unmasks it.
    PS2Keyboard(InterruptController& controller, Threading::Scheduler& scheduler);

    PS2Keyboard(const PS2Keyboard&) = delete;
    PS2Keyboard& operator=(const PS2Keyboard&) = delete;

    char read() override;
    bool tryRead(char& c) override;

    /// What the IRQ handler does with a byte from the controller (public for the self tests).
    void handleScancode(uint8_t byte);

    /// Characters dropped because the buffer was full.
    uint32_t droppedCount() const { return m_dropped; }

    const ScancodeDecoder& decoder() const { return m_decoder; }

private:
    static void onInterrupt(void* keyboard);

    char pop();

    ScancodeDecoder m_decoder;
    char m_buffer[bufferSize] {};
    uint32_t m_head {};
    uint32_t m_count {};
    uint32_t m_dropped {};
    Sync::Semaphore m_available; // one unit per buffered character
};

} // namespace Drivers
