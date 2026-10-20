/**
 * @file character_queue.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::CharacterQueue: characters from interrupt handlers (the
 * keyboard, the serial port) waiting for a reader.
 */

#pragma once

#include <stdint.h>

#include <drivers/character_source.hpp>
#include <sync/semaphore.hpp>

namespace Drivers {

/**
 * @brief A ring buffer of characters plus a semaphore that counts them:
 * push() from an interrupt handler, read() in a thread sleeps until there is
 * one. Several inputs can push into one queue (the shell reads the keyboard
 * and the serial port together). When it is full, new characters are dropped.
 */
class CharacterQueue final : public CharacterSource {
public:
    static constexpr uint32_t capacity = 128;

    explicit CharacterQueue(Threading::Scheduler& scheduler);

    CharacterQueue(const CharacterQueue&) = delete;
    CharacterQueue& operator=(const CharacterQueue&) = delete;

    /// Add one character. Safe in an interrupt handler. false if it was dropped.
    bool push(char c);

    /// Add size characters, all or none (e.g. the escape sequence of an arrow key).
    bool push(const char* data, uint32_t size);

    char read() override;
    bool tryRead(char& c) override;

    uint32_t size() const { return m_count; }

    /// Characters dropped because the queue was full.
    uint32_t droppedCount() const { return m_dropped; }

private:
    char pop();

    char m_buffer[capacity] {};
    uint32_t m_head {};
    uint32_t m_count {};
    uint32_t m_dropped {};
    Sync::Semaphore m_available; // one unit per character in the buffer
};

} // namespace Drivers
