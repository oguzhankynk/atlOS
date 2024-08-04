#include <Drivers/keyboard.hpp>
#include <kernel/sys/IOPort.hpp>
#include <kernel/sys/handlers.hpp>
#include <hardware/pic.hpp>
#include <stdio.h>
#include <string.h>

namespace keyboard
{
    /**
     * @brief Keyboard Scan Codes: Set 1
     * See: https://users.utcluj.ro/~baruch/sie/labor/PS2/Scan_Codes_Set_1.htm
     *
     */
    enum KEYCODE
    {
        NULL_KEY = 0,

        //MAKE Codes
        A_MAKE = 0x1E,
        B_MAKE = 0x30,
        C_MAKE = 0x2E,
        D_MAKE = 0x20,
        E_MAKE = 0x12,
        F_MAKE = 0x21,
        G_MAKE = 0x22,
        H_MAKE = 0x23,
        I_MAKE = 0x17,
        J_MAKE = 0x24,
        K_MAKE = 0x25,
        L_MAKE = 0x26,
        M_MAKE = 0x32,
        N_MAKE = 0x31,
        O_MAKE = 0x18,
        P_MAKE = 0x19,
        Q_MAKE = 0x10,
        R_MAKE = 0x13,
        S_MAKE = 0x1F,
        T_MAKE = 0x14,
        U_MAKE = 0x16,
        V_MAKE = 0x2F,
        W_MAKE = 0x11,
        X_MAKE = 0x2D,
        Y_MAKE = 0x15,
        Z_MAKE = 0x2C,
        // Break Codes
        A_BREAK = 0x9E,
        B_BREAK = 0xB0,
        C_BREAK = 0xAE,
        D_BREAK = 0xA0,
        E_BREAK = 0x92,
        F_BREAK = 0xA1,
        G_BREAK = 0xA2,
        H_BREAK = 0xA3,
        I_BREAK = 0x97,
        J_BREAK = 0xA4,
        K_BREAK = 0xA5,
        L_BREAK = 0xA6,
        M_BREAK = 0xB2,
        N_BREAK = 0xB1,
        O_BREAK = 0x98,
        P_BREAK = 0x99,
        Q_BREAK = 0x90,
        R_BREAK = 0x93,
        S_BREAK = 0x9F,
        T_BREAK = 0x94,
        U_BREAK = 0x96,
        V_BREAK = 0xAF,
        W_BREAK = 0x91,
        X_BREAK = 0xAD,
        Y_BREAK = 0x95,
        Z_BREAK = 0xAC,

        // Make Codes for Numbers
        ZERO_MAKE = 0x0B,
        ONE_MAKE = 0x02,
        TWO_MAKE = 0x03,
        THREE_MAKE = 0x04,
        FOUR_MAKE = 0x05,
        FIVE_MAKE = 0x06,
        SIX_MAKE = 0x07,
        SEVEN_MAKE = 0x08,
        EIGHT_MAKE = 0x09,
        NINE_MAKE = 0x0A,

        // Break Codes for Numbers
        ZERO_BREAK = 0x8B,
        ONE_BREAK = 0x82,
        TWO_BREAK = 0x83,
        THREE_BREAK = 0x84,
        FOUR_BREAK = 0x85,
        FIVE_BREAK = 0x86,
        SIX_BREAK = 0x87,
        SEVEN_BREAK = 0x88,
        EIGHT_BREAK = 0x89,
        NINE_BREAK = 0x8A,

        // Make Codes for Special Characters
        ESC_MAKE = 0x01,
        TAB_MAKE = 0x0F,
        CAPS_LOCK_MAKE = 0x3A,
        LEFT_SHIFT_MAKE = 0x2A,
        RIGHT_SHIFT_MAKE = 0x36,
        LEFT_CTRL_MAKE = 0x1D,
        LEFT_ALT_MAKE = 0x38,
        SPACE_MAKE = 0x39,
        ENTER_MAKE = 0x1C,
        BACKSPACE_MAKE = 0x0E,
        DELETE_MAKE = 0x53,
        LEFT_ARROW_MAKE = 0x4B,
        RIGHT_ARROW_MAKE = 0x4D,
        UP_ARROW_MAKE = 0x48,
        DOWN_ARROW_MAKE = 0x50,
        F1_MAKE = 0x3B,
        F2_MAKE = 0x3C,
        F3_MAKE = 0x3D,
        F4_MAKE = 0x3E,
        F5_MAKE = 0x3F,
        F6_MAKE = 0x40,
        F7_MAKE = 0x41,
        F8_MAKE = 0x42,
        F9_MAKE = 0x43,
        F10_MAKE = 0x44,
        F11_MAKE = 0x57,
        F12_MAKE = 0x58,

        // Break Codes for Special Characters
        ESC_BREAK = 0x81,
        TAB_BREAK = 0x8F,
        CAPS_LOCK_BREAK = 0xBA,
        LEFT_SHIFT_BREAK = 0xAA,
        RIGHT_SHIFT_BREAK = 0xB6,
        LEFT_CTRL_BREAK = 0x9D,
        LEFT_ALT_BREAK = 0xB8,
        SPACE_BREAK = 0xB9,
        ENTER_BREAK = 0x9C,
        BACKSPACE_BREAK = 0x8E,
        DELETE_BREAK = 0xD3,
        LEFT_ARROW_BREAK = 0xCB,
        RIGHT_ARROW_BREAK = 0xCD,
        UP_ARROW_BREAK = 0xC8,
        DOWN_ARROW_BREAK = 0xD0,
        F1_BREAK = 0xBB,
        F2_BREAK = 0xBC,
        F3_BREAK = 0xBD,
        F4_BREAK = 0xBE,
        F5_BREAK = 0xBF,
        F6_BREAK = 0xC0,
        F7_BREAK = 0xC1,
        F8_BREAK = 0xC2,
        F9_BREAK = 0xC3,
        F10_BREAK = 0xC4,
        F11_BREAK = 0xD7,
        F12_BREAK = 0xD8

    };

    struct KeyboardBuffer
    {
        static constexpr uint16_t BUFFER_SIZE = 256;
        int8_t buffer[BUFFER_SIZE]{};
        uint16_t w_index{};
        uint16_t r_index{};

        bool IsFull();
        void Append(int8_t key);
        int8_t getKey();
    };

    static uint8_t ToASCII(uint8_t key);
    static void keyboard_irq(void* Arg);
    static void read_key();

    static KeyboardBuffer key_buffer;

    void Init()
    {
        bool irq_register_stat = Handlers::AddInterruptHandler(33, (interrupt_handle_function)keyboard_irq, nullptr);

        if (irq_register_stat == false)
        {
            printf("keyboard: AddInterruptHandler error \n");
            return;
        }
    }

    int8_t GetKey()
    {
        return key_buffer.getKey();
    }

    static void keyboard_irq(void* Arg)
    {
        key_buffer.Append(ToASCII(IOPort::inportb(0x60)));
        PIC::send_master_eoi(33);
    }

    static uint8_t ToASCII(uint8_t key)
    {
        switch (key)
        {
            // Letters
            case A_MAKE: return 'a';
            case B_MAKE: return 'b';
            case C_MAKE: return 'c';
            case D_MAKE: return 'd';
            case E_MAKE: return 'e';
            case F_MAKE: return 'f';
            case G_MAKE: return 'g';
            case H_MAKE: return 'h';
            case I_MAKE: return 'i';
            case J_MAKE: return 'j';
            case K_MAKE: return 'k';
            case L_MAKE: return 'l';
            case M_MAKE: return 'm';
            case N_MAKE: return 'n';
            case O_MAKE: return 'o';
            case P_MAKE: return 'p';
            case Q_MAKE: return 'q';
            case R_MAKE: return 'r';
            case S_MAKE: return 's';
            case T_MAKE: return 't';
            case U_MAKE: return 'u';
            case V_MAKE: return 'v';
            case W_MAKE: return 'w';
            case X_MAKE: return 'x';
            case Y_MAKE: return 'y';
            case Z_MAKE: return 'z';

            // Numbers
            case ZERO_MAKE: return '0';
            case ONE_MAKE: return '1';
            case TWO_MAKE: return '2';
            case THREE_MAKE: return '3';
            case FOUR_MAKE: return '4';
            case FIVE_MAKE: return '5';
            case SIX_MAKE: return '6';
            case SEVEN_MAKE: return '7';
            case EIGHT_MAKE: return '8';
            case NINE_MAKE: return '9';

            // Special Characters
            case SPACE_MAKE: return ' ';
            case ENTER_MAKE: return '\n';
            case TAB_MAKE: return '\t';
            case ESC_MAKE: return 27; // ASCII for ESC

            // Other special characters could be added similarly
            default: return '\0'; // Null character for unhandled cases
        }
    }

    bool KeyboardBuffer::IsFull()
    {
        return w_index == (r_index + BUFFER_SIZE - 1) % BUFFER_SIZE;
    }

    void KeyboardBuffer::Append(int8_t key)
    {
        if (key == 0)
            return;

        if (!IsFull())
        {
            buffer[w_index] = key;
            w_index = (w_index + 1) % BUFFER_SIZE;
        }
    }

    int8_t KeyboardBuffer::getKey()
    {
        int8_t c = 0;

        if (w_index != r_index)
        {
            c = buffer[r_index];
            r_index = (r_index + 1) % BUFFER_SIZE;
        }

        return c;

    }

}; // namespace keyboard
