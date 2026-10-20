/**
 * @file scancode.hpp
 * @author Oguzhan KAYNAK
 * @brief PS/2 scancode set 1 to key events and characters. No hardware access,
 * so the self tests can feed it bytes.
 *
 * The controller translates whatever the keyboard sends to set 1 (the
 * default, also with USB legacy emulation): one byte per key press ("make"),
 * the same byte | 0x80 on release ("break"), 0xE0 before keys that came later
 * (right Ctrl, arrows, keypad Enter, ...).
 */

#pragma once

#include <stdint.h>

namespace Drivers {

/// @brief Characters for each set 1 make code, without and with Shift.
struct KeyboardLayout {
    const char* normal;
    const char* shifted;
    uint8_t size; ///< Number of make codes in both tables
};

/// @brief US QWERTY.
const KeyboardLayout& usLayout();

struct KeyEvent {
    uint8_t code; ///< Make code (bit 7 cleared)
    bool extended; ///< Came after 0xE0
    bool pressed; ///< false on release
    char character; ///< 0 for keys without one (Shift, F1, arrows, releases)
};

/**
 * @brief Turns scancode bytes into key events, keeping track of Shift, Ctrl,
 * Alt and Caps Lock.
 */
class ScancodeDecoder {
public:
    explicit ScancodeDecoder(const KeyboardLayout& layout = usLayout());

    /**
     * @brief Feed one byte from the controller.
     * @return true if it completed a key event (written to event); false for
     * prefixes and bytes that are not keys (acknowledgements, errors, Pause).
     */
    bool feed(uint8_t byte, KeyEvent& event);

    bool shift() const { return m_leftShift || m_rightShift; }
    bool ctrl() const { return m_ctrl; }
    bool alt() const { return m_alt; }
    bool capsLock() const { return m_capsLock; }

private:
    char character(uint8_t code, bool extended) const;

    const KeyboardLayout& m_layout;
    bool m_extended {};
    uint8_t m_skip {}; // bytes of a Pause sequence still to drop
    bool m_leftShift {};
    bool m_rightShift {};
    bool m_ctrl {};
    bool m_alt {};
    bool m_capsLock {};
};

} // namespace Drivers
