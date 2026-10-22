/**
 * @file tick_source.hpp
 * @brief Interface of a periodic timer: the local APIC timer, or the PIT.
 */

#pragma once

#include <stdint.h>

namespace Drivers {

class TickSource {
public:
    using TickHandler = void (*)(void*);

    /**
     * @brief Call handler(arg) every periodNs nanoseconds, from interrupt context.
     * @return false if the period is not supported; nothing is started then.
     */
    virtual bool start(uint32_t periodNs, TickHandler handler, void* arg) = 0;

protected:
    // Never deleted through the interface
    ~TickSource() = default;
};

} // namespace Drivers
