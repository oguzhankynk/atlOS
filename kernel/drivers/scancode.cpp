/**
 * @file scancode.cpp
 * @author Oguzhan KAYNAK
 * @brief PS/2 scancode set 1 to key events and characters.
 */

#include <drivers/scancode.hpp>

namespace Drivers {

// Index = make code. 0x3B..0x44 are F1..F10, 0x45/0x46 Num/Scroll Lock,
// 0x47..0x53 the keypad (always digits: Num Lock is not tracked).
static constexpr char usNormal[] = "\0\x1b"
                                   "1234567890-=\b\t"
                                   "qwertyuiop[]\n\0"
                                   "asdfghjkl;'`\0\\"
                                   "zxcvbnm,./\0*\0 \0"
                                   "\0\0\0\0\0\0\0\0\0\0\0\0"
                                   "789-456+1230.";

static constexpr char usShifted[] = "\0\x1b"
                                    "!@#$%^&*()_+\b\t"
                                    "QWERTYUIOP{}\n\0"
                                    "ASDFGHJKL:\"~\0|"
                                    "ZXCVBNM<>?\0*\0 \0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "789-456+1230.";

static_assert(sizeof(usNormal) == 0x54 + 1, "make codes 0x00..0x53");
static_assert(sizeof(usShifted) == sizeof(usNormal), "same size tables");

const KeyboardLayout& usLayout()
{
    static const KeyboardLayout s_layout = { usNormal, usShifted, sizeof(usNormal) - 1 };
    return s_layout;
}

enum : uint8_t {
    Extended = 0xE0,
    Pause = 0xE1, // E1 1D 45 E1 9D C5: no release, no character
    Released = 0x80,

    LeftShift = 0x2A,
    RightShift = 0x36,
    Ctrl = 0x1D, // right Ctrl: E0 1D
    Alt = 0x38, // right Alt: E0 38
    CapsLock = 0x3A,
    Enter = 0x1C,
    Slash = 0x35,
};

static bool isLetter(char c)
{
    return c >= 'a' && c <= 'z';
}

ScancodeDecoder::ScancodeDecoder(const KeyboardLayout& layout)
    : m_layout(layout)
{
}

char ScancodeDecoder::character(uint8_t code, bool extended) const
{
    // Of the extended keys only keypad Enter and keypad / have a character
    if (extended) {
        if (code == Enter)
            return '\n';
        if (code == Slash)
            return '/';
        return 0;
    }

    if (code >= m_layout.size)
        return 0;

    char base = m_layout.normal[code];

    // Ctrl+letter: the control characters 1..26 (Ctrl+C = 3)
    if (m_ctrl && isLetter(base))
        return static_cast<char>(base - 'a' + 1);

    // Caps Lock only changes letters, and Shift undoes it
    bool upper = shift();
    if (m_capsLock && isLetter(base))
        upper = !upper;

    return upper ? m_layout.shifted[code] : base;
}

bool ScancodeDecoder::feed(uint8_t byte, KeyEvent& event)
{
    if (m_skip) {
        m_skip--;
        return false;
    }

    if (byte == Extended) {
        m_extended = true;
        return false;
    }

    if (byte == Pause) {
        m_skip = 5;
        return false;
    }

    // 0x00 and 0xFF: errors; 0xFA, 0xFE: acknowledge, resend (answers to commands)
    if (byte == 0x00 || byte == 0xFF || byte == 0xFA || byte == 0xFE) {
        m_extended = false;
        return false;
    }

    event.code = byte & ~Released;
    event.pressed = !(byte & Released);
    event.extended = m_extended;
    m_extended = false;

    switch (event.code) {
    case LeftShift:
    case RightShift:
        // E0 2A / E0 AA are fake shifts around Print Screen: ignore
        if (!event.extended)
            (event.code == LeftShift ? m_leftShift : m_rightShift) = event.pressed;
        break;
    case Ctrl:
        m_ctrl = event.pressed;
        break;
    case Alt:
        m_alt = event.pressed;
        break;
    case CapsLock:
        if (event.pressed)
            m_capsLock = !m_capsLock;
        break;
    default:
        break;
    }

    event.character = event.pressed ? character(event.code, event.extended) : static_cast<char>(0);
    return true;
}

} // namespace Drivers
