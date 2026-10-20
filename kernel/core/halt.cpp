/**
 * @file halt.cpp
 * @author Oguzhan KAYNAK
 * @brief Stopping the machine for good.
 */

#include <core/halt.hpp>

#include <arch/x86_64/io/ioport.hpp>

namespace Core {

void halt()
{
    // hlt wakes up on an NMI even with interrupts off, so loop
    while (true)
        __asm__ volatile("cli; hlt");
}

void haltWithStatus(ExitStatus status)
{
#ifdef ATLOS_DEBUG_EXIT_PORT
    IOPort::outportb(ATLOS_DEBUG_EXIT_PORT, static_cast<uint8_t>(status));
#endif
    halt();
}

} // namespace Core
