/**
 * @file handlers.hpp
 * @author Oguzhan KAYNAK ()
 * @brief This file contains the definitions and declarations of event handlers
 * and interrupt handlers used in the operating system.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once

#include <stdint.h>

/**
 * @brief A function pointer type for exception handling functions.
 *
 */
typedef void (*InterruptHandlerFunction)(void*);

namespace Drivers {
class InterruptController;
}

namespace Handlers {

/**
 * @brief Initializes the Handler
 *
 */
void init();

/**
 * @brief Registers an Interrupt handler for a specific exception number.
 *
 * @param exceptionNo The Interrupt number to register the handler for.
 * @param function The function to be called when the specified exception occurs.
 * @param arg The argument to be passed to the registered handler function
 * @return true if the Interrupt handler was successfully registered
 * @return false otherwise
 */
bool addInterruptHandler(uint32_t exceptionNo, InterruptHandlerFunction function, void* arg);

/**
 * @brief Set the controller that acknowledges IRQs (sends EOI).
 *
 * This is the boundary between the assembly ISR stubs, which can not be given
 * an object, and the interrupt controller: kernelMain binds it once.
 */
void setInterruptController(Drivers::InterruptController* controller);

/**
 * @brief Call the Interrupt Handler function
 *
 * @param exceptionNo
 */
void interruptHandler(uint32_t exceptionNo);

}; // namespace Handlers
