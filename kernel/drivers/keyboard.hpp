#pragma once

#include <stdint.h>

namespace Drivers {
class InterruptController;
}

namespace Keyboard {
void init(Drivers::InterruptController& controller);
int8_t getKey();

}; // namespace Keyboard
