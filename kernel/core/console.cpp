#include <core/console.hpp>
#include <drivers/character_sink.hpp>
#include <stdio.h>

namespace Core {

// Bound by kernelMain, see setConsole
static Drivers::CharacterSink* s_console {};

static void writeToConsole(const char* data, size_t size)
{
    if (s_console)
        s_console->write(data, size);
}

Drivers::CharacterSink* setConsole(Drivers::CharacterSink& sink)
{
    Drivers::CharacterSink* previous = s_console;
    s_console = &sink;
    set_write_handler(writeToConsole);
    return previous;
}

} // namespace Core
