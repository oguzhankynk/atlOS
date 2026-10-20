/**
 * @file interrupt_guard.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::InterruptGuard: interrupts off for the lifetime of a scope.
 *
 * On one CPU with a cooperative scheduler, the only code that can run in the
 * middle of a thread is an interrupt handler. Turning interrupts off makes a
 * few steps that a handler also touches (a counter, a wait queue) one step.
 * Guards nest: each one restores the state it found.
 */

#pragma once

#include <arch/x86_64/io/ioport.hpp>

namespace Sync {

class InterruptGuard {
public:
    InterruptGuard()
        : m_wasEnabled(IOPort::interruptsEnabled())
    {
        IOPort::disableInterrupts();
    }

    ~InterruptGuard()
    {
        if (m_wasEnabled)
            IOPort::enableInterrupts();
    }

    InterruptGuard(const InterruptGuard&) = delete;
    InterruptGuard& operator=(const InterruptGuard&) = delete;

private:
    bool m_wasEnabled;
};

} // namespace Sync
