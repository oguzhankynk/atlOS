/**
 * @file banner.cpp
 * @author Oguzhan KAYNAK
 * @brief atlOS banner printer
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
#include <core/banner.hpp>
#include <core/version.hpp>
#include <stdio.h>

// ANSI colors: serial terminals show them, VGATextConsole understands them too
#define NAME "\x1b[93m" // bright yellow
#define BRIGHT "\x1b[97m" // bright white
#define DIM "\x1b[37m"
#define RESET "\x1b[0m"

// Plain ASCII: the same on the VGA screen and on any serial terminal
static const char* const s_banner[] = {
    NAME "          _   _  _____  _____ " RESET,
    NAME "         | | | ||  _  |/  ___|" RESET,
    NAME "     __ _| |_| || | | |\\ `--. " RESET,
    NAME "    / _` | __| || | | | `--. \\" RESET,
    NAME "   | (_| | |_| |\\ \\_/ //\\__/ /" RESET,
    NAME "    \\__,_|\\__|_| \\___/ \\____/ " RESET,
};

void atlOSBootLine()
{
    printf("atlOS %s (%s) starting\n", ATLOS_VERSION_STRING, ATLOS_GIT_DESCRIBE);
}

void atlOSBanner()
{
    printf("\n");
    for (const char* line : s_banner)
        printf("%s\n", line);
    printf("\n  " BRIGHT "atlOS %s" RESET DIM " (%s)  x86_64, a small kernel to learn how an OS works" RESET "\n\n",
        ATLOS_VERSION_STRING, ATLOS_GIT_DESCRIBE);
}
