/**
 * @file character_sink.hpp
 * @brief Interface of something characters can be written to (serial port, VGA text screen).
 */

#pragma once

#include <stddef.h>

namespace Drivers {

class CharacterSink {
public:
    virtual void write(const char* data, size_t size) = 0;

    void write(char c) { write(&c, 1); }

protected:
    // Never deleted through the interface
    ~CharacterSink() = default;
};

} // namespace Drivers
