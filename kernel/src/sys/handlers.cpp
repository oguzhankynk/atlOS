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

#include <kernel/sys/handlers.hpp>

#include <stdio.h>
#include <arch/i386/IDT/idt.hpp>

static exception_handle_function FUNCS[IDT::IDT_TABLE_LEN];
static void *ARGS[IDT::IDT_TABLE_LEN];

void DEFAULT_HANDLER_FUNCTION(void *param)
{
    printf("DEFAULT_HANDLER_FUNCTION \n");
}

/**
 * @brief Initializes the Handler
 *
 */
void Handlers::Init()
{
    for (auto i = 0; i < IDT::IDT_TABLE_LEN; i++)
    {
        FUNCS[i] = (exception_handle_function)DEFAULT_HANDLER_FUNCTION;
        ARGS[i] = nullptr;
    }
}

/**
 * @brief Registers an exception handler for a specific exception number.
 *
 * @param exception_no The exception number to register the handler for.
 * @param function The function to be called when the specified exception occurs.
 * @param Arg The argument to be passed to the registered handler function
 * @return true if the exception handler was successfully registered
 * @return false otherwise
 */
bool Handlers::AddExceptionHandler(uint32_t exception_no, exception_handle_function function, void *Arg)
{
    if (exception_no >= IDT::IDT_TABLE_LEN)
        return false;

    FUNCS[exception_no] = function;
    ARGS[exception_no] = Arg;

    return true;
}

/**
 * @brief Call the Exception Handler function
 *
 * @param exception_no
 */
void Handlers::ExceptionHandler(uint32_t exception_no)
{
    if (exception_no >= IDT::IDT_TABLE_LEN)
        return ;

    auto fn = FUNCS[exception_no];
    auto arg = ARGS[exception_no];

    fn(arg);
}