/**
 * @file character_queue.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::CharacterQueue: characters waiting for a reader.
 */

#include <drivers/character_queue.hpp>

#include <sync/interrupt_guard.hpp>

namespace Drivers {

CharacterQueue::CharacterQueue(Threading::Scheduler& scheduler)
    : m_available(scheduler, 0, capacity)
{
}

bool CharacterQueue::push(char c)
{
    return push(&c, 1);
}

bool CharacterQueue::push(const char* data, uint32_t size)
{
    // Threads push too (the self tests): the buffer is shared with the IRQs
    Sync::InterruptGuard guard;

    if (size > capacity - m_count) {
        m_dropped += size;
        return false;
    }

    for (uint32_t i = 0; i < size; i++) {
        m_buffer[(m_head + m_count) % capacity] = data[i];
        m_count++;
        m_available.give();
    }
    return true;
}

char CharacterQueue::pop()
{
    Sync::InterruptGuard guard;

    char c = m_buffer[m_head];
    m_head = (m_head + 1) % capacity;
    m_count--;
    return c;
}

char CharacterQueue::read()
{
    // A unit means a character is in the buffer and nobody else will pop it
    m_available.take();
    return pop();
}

bool CharacterQueue::tryRead(char& c)
{
    if (!m_available.tryTake())
        return false;
    c = pop();
    return true;
}

} // namespace Drivers
