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

namespace Handlers
{

    static interrupt_handle_function FUNCS[IDT::IDT_TABLE_LEN];
    static void *ARGS[IDT::IDT_TABLE_LEN];

    /**
     * @brief Default handler function
     *
     * @param param
     */
    static void DEFAULT_HANDLER_FUNCTION(void *param)
    {
        printf("DEFAULT_HANDLER_FUNCTION: %x \n", (uintptr_t)param);
    }

    /**
     * @brief Initializes the Handler
     *
     */
    void Init()
    {
        for (auto i = 0; i < IDT::IDT_TABLE_LEN; i++)
        {
            AddInterruptHandler(i, (interrupt_handle_function)DEFAULT_HANDLER_FUNCTION, reinterpret_cast<void *>(i));
        }
    }

    /**
     * @brief Registers an Interrupt handler for a specific exception number.
     *
     * @param exception_no The Interrupt number to register the handler for.
     * @param function The function to be called when the specified exception occurs.
     * @param Arg The argument to be passed to the registered handler function
     * @return true if the Interrupt handler was successfully registered
     * @return false otherwise
     */
    bool AddInterruptHandler(uint32_t exception_no, interrupt_handle_function function, void *Arg)
    {
        if (exception_no >= IDT::IDT_TABLE_LEN)
            return false;

        FUNCS[exception_no] = function;
        ARGS[exception_no] = Arg;

        return true;
    }

    /**
     * @brief Call the Interrupt Handler function
     *
     * @param exception_no
     */
    void InterruptHandler(uint32_t exception_no)
    {
        if (exception_no >= IDT::IDT_TABLE_LEN)
            return;

        // Send EOI if exception_no is in the range of IRQs (32-47)
        if (exception_no >= 32 && exception_no < 48)
        {
            PIC::send_eoi(static_cast<PIC::IRQ>(exception_no - 32));
        }
        auto fn = FUNCS[exception_no];
        auto arg = ARGS[exception_no];
        fn(arg);
    }

}; // namespace Handlers