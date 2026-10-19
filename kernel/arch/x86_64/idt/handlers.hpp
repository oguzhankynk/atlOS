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
#ifndef CA54FFDF_8049_47E3_9ED3_76405AD1CF26
#define CA54FFDF_8049_47E3_9ED3_76405AD1CF26

#include <stdint.h>

/**
 * @brief A function pointer type for exception handling functions.
 *
 */
typedef void (*interrupt_handle_function)(void *);

namespace Handlers
{

    /**
     * @brief Initializes the Handler
     *
     */
    void Init();

    /**
     * @brief Registers an Interrupt handler for a specific exception number.
     *
     * @param exception_no The Interrupt number to register the handler for.
     * @param function The function to be called when the specified exception occurs.
     * @param Arg The argument to be passed to the registered handler function
     * @return true if the Interrupt handler was successfully registered
     * @return false otherwise
     */
    bool AddInterruptHandler(uint32_t exception_no, interrupt_handle_function function, void *Arg);

    /**
     * @brief Call the Interrupt Handler function
     *
     * @param exception_no
     */
    void InterruptHandler(uint32_t exception_no);

}; // namespace Handlers

#endif /* CA54FFDF_8049_47E3_9ED3_76405AD1CF26 */
