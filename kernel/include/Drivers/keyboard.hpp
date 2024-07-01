#ifndef D75DF66C_37A7_48DA_AA5C_A6C44B102C0F
#define D75DF66C_37A7_48DA_AA5C_A6C44B102C0F

#include <stdint.h>

namespace keyboard
{
    void Init();
    bool Isenabled();
    int8_t GetKey();
    uint8_t ToASCII(uint8_t key);
    
};//namespace keyboard
#endif /* D75DF66C_37A7_48DA_AA5C_A6C44B102C0F */
