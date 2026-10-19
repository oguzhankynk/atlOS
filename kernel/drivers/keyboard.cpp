#include <arch/x86_64/io/ioport.hpp>
#include <drivers/keyboard.hpp>
#include <drivers/pic.hpp>
#include <stdio.h>
#include <string.h>

namespace Keyboard {
/**
 * @brief Keyboard Scan Codes: Set 1
 * See: https://users.utcluj.ro/~baruch/sie/labor/PS2/Scan_Codes_Set_1.htm
 *
 */
enum KEYCODE {
    NullKey = 0,

    // MAKE Codes
    AMake = 0x1E,
    BMake = 0x30,
    CMake = 0x2E,
    DMake = 0x20,
    EMake = 0x12,
    FMake = 0x21,
    GMake = 0x22,
    HMake = 0x23,
    IMake = 0x17,
    JMake = 0x24,
    KMake = 0x25,
    LMake = 0x26,
    MMake = 0x32,
    NMake = 0x31,
    OMake = 0x18,
    PMake = 0x19,
    QMake = 0x10,
    RMake = 0x13,
    SMake = 0x1F,
    TMake = 0x14,
    UMake = 0x16,
    VMake = 0x2F,
    WMake = 0x11,
    XMake = 0x2D,
    YMake = 0x15,
    ZMake = 0x2C,
    // Break Codes
    ABreak = 0x9E,
    BBreak = 0xB0,
    CBreak = 0xAE,
    DBreak = 0xA0,
    EBreak = 0x92,
    FBreak = 0xA1,
    GBreak = 0xA2,
    HBreak = 0xA3,
    IBreak = 0x97,
    JBreak = 0xA4,
    KBreak = 0xA5,
    LBreak = 0xA6,
    MBreak = 0xB2,
    NBreak = 0xB1,
    OBreak = 0x98,
    PBreak = 0x99,
    QBreak = 0x90,
    RBreak = 0x93,
    SBreak = 0x9F,
    TBreak = 0x94,
    UBreak = 0x96,
    VBreak = 0xAF,
    WBreak = 0x91,
    XBreak = 0xAD,
    YBreak = 0x95,
    ZBreak = 0xAC,

    // Make Codes for Numbers
    ZeroMake = 0x0B,
    OneMake = 0x02,
    TwoMake = 0x03,
    ThreeMake = 0x04,
    FourMake = 0x05,
    FiveMake = 0x06,
    SixMake = 0x07,
    SevenMake = 0x08,
    EightMake = 0x09,
    NineMake = 0x0A,

    // Break Codes for Numbers
    ZeroBreak = 0x8B,
    OneBreak = 0x82,
    TwoBreak = 0x83,
    ThreeBreak = 0x84,
    FourBreak = 0x85,
    FiveBreak = 0x86,
    SixBreak = 0x87,
    SevenBreak = 0x88,
    EightBreak = 0x89,
    NineBreak = 0x8A,

    // Make Codes for Special Characters
    EscMake = 0x01,
    TabMake = 0x0F,
    CapsLockMake = 0x3A,
    LeftShiftMake = 0x2A,
    RightShiftMake = 0x36,
    LeftCtrlMake = 0x1D,
    LeftAltMake = 0x38,
    SpaceMake = 0x39,
    EnterMake = 0x1C,
    BackspaceMake = 0x0E,
    DeleteMake = 0x53,
    LeftArrowMake = 0x4B,
    RightArrowMake = 0x4D,
    UpArrowMake = 0x48,
    DownArrowMake = 0x50,
    F1Make = 0x3B,
    F2Make = 0x3C,
    F3Make = 0x3D,
    F4Make = 0x3E,
    F5Make = 0x3F,
    F6Make = 0x40,
    F7Make = 0x41,
    F8Make = 0x42,
    F9Make = 0x43,
    F10Make = 0x44,
    F11Make = 0x57,
    F12Make = 0x58,

    // Break Codes for Special Characters
    EscBreak = 0x81,
    TabBreak = 0x8F,
    CapsLockBreak = 0xBA,
    LeftShiftBreak = 0xAA,
    RightShiftBreak = 0xB6,
    LeftCtrlBreak = 0x9D,
    LeftAltBreak = 0xB8,
    SpaceBreak = 0xB9,
    EnterBreak = 0x9C,
    BackspaceBreak = 0x8E,
    DeleteBreak = 0xD3,
    LeftArrowBreak = 0xCB,
    RightArrowBreak = 0xCD,
    UpArrowBreak = 0xC8,
    DownArrowBreak = 0xD0,
    F1Break = 0xBB,
    F2Break = 0xBC,
    F3Break = 0xBD,
    F4Break = 0xBE,
    F5Break = 0xBF,
    F6Break = 0xC0,
    F7Break = 0xC1,
    F8Break = 0xC2,
    F9Break = 0xC3,
    F10Break = 0xC4,
    F11Break = 0xD7,
    F12Break = 0xD8

};

struct KeyboardBuffer {
    static constexpr uint16_t bufferSize = 256;
    int8_t buffer[bufferSize] {};
    uint16_t w_index {};
    uint16_t r_index {};

    bool isFull();
    void append(int8_t key);
    int8_t getKey();
};

static uint8_t toAscii(uint8_t key);
static void keyboardIrq(void* arg);
static void readKey();

static KeyboardBuffer s_keyBuffer;

void init()
{
    bool irqRegisterStat = PIC::addIrqHandler(PIC::IRQ::Keyboard, keyboardIrq, (void*)nullptr);

    if (irqRegisterStat == false) {
        printf("Keyboard: addIrqHandler error \n");
        return;
    }

    PIC::irqEnable(PIC::IRQ::Keyboard); // Enable Keyboard interrupt
}

int8_t getKey()
{
    return s_keyBuffer.getKey();
}

static void keyboardIrq(void* arg)
{
    s_keyBuffer.append(toAscii(IOPort::inportb(0x60)));
}

static uint8_t toAscii(uint8_t key)
{
    switch (key) {
    // Letters
    case AMake:
        return 'a';
    case BMake:
        return 'b';
    case CMake:
        return 'c';
    case DMake:
        return 'd';
    case EMake:
        return 'e';
    case FMake:
        return 'f';
    case GMake:
        return 'g';
    case HMake:
        return 'h';
    case IMake:
        return 'i';
    case JMake:
        return 'j';
    case KMake:
        return 'k';
    case LMake:
        return 'l';
    case MMake:
        return 'm';
    case NMake:
        return 'n';
    case OMake:
        return 'o';
    case PMake:
        return 'p';
    case QMake:
        return 'q';
    case RMake:
        return 'r';
    case SMake:
        return 's';
    case TMake:
        return 't';
    case UMake:
        return 'u';
    case VMake:
        return 'v';
    case WMake:
        return 'w';
    case XMake:
        return 'x';
    case YMake:
        return 'y';
    case ZMake:
        return 'z';

    // Numbers
    case ZeroMake:
        return '0';
    case OneMake:
        return '1';
    case TwoMake:
        return '2';
    case ThreeMake:
        return '3';
    case FourMake:
        return '4';
    case FiveMake:
        return '5';
    case SixMake:
        return '6';
    case SevenMake:
        return '7';
    case EightMake:
        return '8';
    case NineMake:
        return '9';

    // Special Characters
    case SpaceMake:
        return ' ';
    case EnterMake:
        return '\n';
    case TabMake:
        return '\t';
    case EscMake:
        return 27; // ASCII for ESC

    // Other special characters could be added similarly
    default:
        return '\0'; // Null character for unhandled cases
    }
}

bool KeyboardBuffer::isFull()
{
    return w_index == (r_index + bufferSize - 1) % bufferSize;
}

void KeyboardBuffer::append(int8_t key)
{
    if (key == 0)
        return;

    if (!isFull()) {
        buffer[w_index] = key;
        w_index = (w_index + 1) % bufferSize;
    }
}

int8_t KeyboardBuffer::getKey()
{
    int8_t c = 0;

    if (w_index != r_index) {
        c = buffer[r_index];
        r_index = (r_index + 1) % bufferSize;
    }

    return c;
}

}; // namespace Keyboard
