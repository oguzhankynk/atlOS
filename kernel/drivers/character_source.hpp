/**
 * @file character_source.hpp
 * @author Oguzhan KAYNAK
 * @brief Interface of something characters can be read from (the keyboard
 * today, the serial port later). The input side of CharacterSink.
 */

#pragma once

namespace Drivers {

class CharacterSource {
public:
    /// Wait for the next character. Threads only.
    virtual char read() = 0;

    /// Take the next character if there is one, never wait.
    virtual bool tryRead(char& c) = 0;

protected:
    // Never deleted through the interface
    ~CharacterSource() = default;
};

} // namespace Drivers
