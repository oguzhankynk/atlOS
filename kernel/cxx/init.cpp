/**
 * @file init.cpp
 * @author Oguzhan KAYNAK ()
 * @brief
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

#include <cxx/init.hpp>

#include <mm/constants.hpp>

namespace cxx
{
    /**
     * @brief Initialize all global/static C++ constructors.
     *
     */
    void callConstructors()
    {
        for (auto ctor = Memory::__ctors_start; ctor < Memory::__ctors_end; ++ctor)
        {
            if (*ctor == reinterpret_cast<void (*)(void)>(-1) || *ctor == nullptr)
                continue;
            (*ctor)();
        }
    }

    /**
     * @brief Call all global/static C++ destructors.
     *
     */
    void callDestructors()
    {
        for (auto dtor = Memory::__dtors_start; dtor < Memory::__dtors_end; ++dtor)
        {
            if (*dtor == reinterpret_cast<void (*)(void)>(-1) || *dtor == nullptr)
                continue;
            (*dtor)();
        }
    }
}

