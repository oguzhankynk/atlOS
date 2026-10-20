/**
 * @file tee_sink.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::TeeSink: write to two sinks at once (the screen and the
 * serial port), like the Unix tee command.
 */

#pragma once

#include <drivers/character_sink.hpp>

namespace Drivers {

class TeeSink final : public CharacterSink {
public:
    TeeSink(CharacterSink& first, CharacterSink& second)
        : m_first(first)
        , m_second(second)
    {
    }

    void write(const char* data, size_t size) override
    {
        m_first.write(data, size);
        m_second.write(data, size);
    }

private:
    CharacterSink& m_first;
    CharacterSink& m_second;
};

} // namespace Drivers
