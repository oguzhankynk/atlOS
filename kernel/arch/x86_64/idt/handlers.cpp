/**
 * @file handlers.cpp
 * @author Oguzhan KAYNAK ()
 * @brief This file contains the implemantation of event handlers
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

#include <arch/x86_64/idt/handlers.hpp>

#include <stdio.h>

#include <arch/x86_64/idt/idt.hpp>
#include <drivers/pic.hpp>

namespace Handlers {

static InterruptHandlerFunction s_funcs[IDT::idtTableLen];
static void* s_args[IDT::idtTableLen];

/**
 * @brief Default handler function
 *
 * @param param
 */
static void defaultHandlerFunction(void* param)
{
    printf("defaultHandlerFunction: %x \n", (uintptr_t)param);
}

/**
 * @brief Initializes the Handler
 *
 */
void init()
{
    for (auto i = 0; i < IDT::idtTableLen; i++) {
        addInterruptHandler(i, (InterruptHandlerFunction)defaultHandlerFunction, reinterpret_cast<void*>(i));
    }
}

/**
 * @brief Registers an Interrupt handler for a specific exception number.
 *
 * @param exceptionNo The Interrupt number to register the handler for.
 * @param function The function to be called when the specified exception occurs.
 * @param arg The argument to be passed to the registered handler function
 * @return true if the Interrupt handler was successfully registered
 * @return false otherwise
 */
bool addInterruptHandler(uint32_t exceptionNo, InterruptHandlerFunction function, void* arg)
{
    if (exceptionNo >= IDT::idtTableLen)
        return false;

    s_funcs[exceptionNo] = function;
    s_args[exceptionNo] = arg;

    return true;
}

/**
 * @brief Call the Interrupt Handler function
 *
 * @param exceptionNo
 */
void interruptHandler(uint32_t exceptionNo)
{
    if (exceptionNo >= IDT::idtTableLen)
        return;

    // Send EOI if exception_no is in the range of IRQs (32-47)
    if (exceptionNo >= 32 && exceptionNo < 48) {
        PIC::sendEoi(static_cast<PIC::IRQ>(exceptionNo - 32));
    }
    auto fn = s_funcs[exceptionNo];
    auto arg = s_args[exceptionNo];
    fn(arg);
}

}; // namespace Handlers