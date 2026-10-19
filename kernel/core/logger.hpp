/**
 * @file logger.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Logging utility that allows printing formatted messages
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

#ifndef CA43A890_B7F4_4423_83C8_F73B23F5AA52
#define CA43A890_B7F4_4423_83C8_F73B23F5AA52

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Enum class to control logging behavior.
 *
 * This enum defines two states:
 * - LogFlag::Log: Enable logging.
 * - LogFlag::NoLog: Disable logging.
 */
enum class LogFlag
{
    NoLog, ///< Disables logging.
    Log    ///< Enables logging.
};

/**
 * @brief Logs a formatted message if the flag is set to LogFlag::Log.
 * If the flag is set to LogFlag::NoLog, the function does nothing.
 *
 * @param flag If LogFlag::NoLog, the function does nothing. If LogFlag::Log, it prints the formatted message.
 * @param format A C-string that contains the format of the message, similar to `printf`.
 * @param ... A variable number of arguments to match the format specifiers in the format string.
 *
 * @note This function uses variadic arguments to support formatted messages.
 */
static inline void LOG_MSG(LogFlag flag, const char *format, ...)
{
    if (flag == LogFlag::NoLog)
    {
        return; // Do nothing
    }

    if (flag == LogFlag::Log)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args); // Print the formatted message
        va_end(args);
    }
}

#endif /* CA43A890_B7F4_4423_83C8_F73B23F5AA52 */
