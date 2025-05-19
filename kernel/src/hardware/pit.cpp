/**
 * @file pit.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Implemantation for interfacing with the the (Intel 8253/8254) Programmable Interval Timer (PIT)
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

#include <hardware/pit.hpp>
#include <kernel/sys/IOPort.hpp>
#include <hardware/pic.hpp>
#include <kernel/sys/handlers.hpp>
#include <stdio.h>

namespace PIT
{
    /**
     * @brief Base frequency of the PIT
     */
    static constexpr auto PIT_BASE_FREQUENCY = 1193182ULL;

    /**
     * @brief I/O port for channel 0
     */
    static constexpr uint16_t PIT_CHANNEL_0 = 0x40;

    /**
     * @brief I/O port for channel 1
     */
    static constexpr uint16_t PIT_CHANNEL_1 = 0x40;

    /**
     * @brief I/O port for channel 2
     */
    static constexpr uint16_t PIT_CHANNEL_2 = 0x40;

    /**
     * @brief Command port for the PIT
     */
    static constexpr uint16_t PIT_COMMAND_PORT = 0x43;

    /**
     * @brief Select channel
     */
    enum class PITChannel : uint8_t
    {
        Channel0 = 0b00 << 6, /**< Channel 0 */
        Channel1 = 0b01 << 6, /**< Channel 1 */
        Channel2 = 0b10 << 6, /**< Channel 2 */
        ReadBack = 0b11 << 6  /**< Read-back command (8254 only) */
    };

    /**
     * @brief Access mode
     */
    enum class PITAccessMode : uint8_t
    {
        LatchCount = 0b00 << 4,  /**< Latch count value */
        LobyteOnly = 0b01 << 4,  /**< Access low byte only */
        HibyteOnly = 0b10 << 4,  /**< Access high byte only */
        LobyteHibyte = 0b11 << 4 /**< Access low byte, then high byte */
    };

    /**
     * @brief Operating mode
     */
    enum class PITOperatingMode : uint8_t
    {
        Mode0 = 0b000 << 1,    /**< Interrupt on terminal count */
        Mode1 = 0b001 << 1,    /**< Hardware re-triggerable one-shot */
        Mode2 = 0b010 << 1,    /**< Rate generator */
        Mode3 = 0b011 << 1,    /**< Square wave generator */
        Mode4 = 0b100 << 1,    /**< Software triggered strobe */
        Mode5 = 0b101 << 1,    /**< Hardware triggered strobe */
        Mode2Alt = 0b110 << 1, /**< Rate generator (same as 010) */
        Mode3Alt = 0b111 << 1  /**< Square wave generator (same as 011) */
    };

    /**
     * @brief Binary Coded Decimal (BCD) mode
     */

    enum class PITBCDMode : uint8_t
    {
        Binary = 0b0, /**< 16-bit binary */
        BCD = 0b1     /**< Four-digit BCD */
    };

    /**
     * @brief Control word for the PIT
     */
    struct PITControlWord
    {
        PITChannel channel;
        PITAccessMode accessMode;
        PITOperatingMode operatingMode;
        PITBCDMode bcdMode;

        uint8_t toByte() const
        {
            return static_cast<uint8_t>(channel) |
                   static_cast<uint8_t>(accessMode) |
                   static_cast<uint8_t>(operatingMode) |
                   static_cast<uint8_t>(bcdMode);
        }
    };

    static void pitHandler(void *args)
    {
        static uint32_t count = 0;
        // printf("PIT interruptaaaaaaaaaaaaaaaa : %d \n", count++);
    }

    /**
     * @brief Initializes the PIT8253/8254 with the desired settings.
     *
     * @param period_ns The desired period in nanoseconds.
     */
    void init(uint32_t period_ns)
    {

        uint32_t divisor = (PIT_BASE_FREQUENCY * period_ns) / 1000000000;

        // Control Word ayarla (Channel 0, Lobyte/Hibyte, Square Wave, Binary)
        PITControlWord cmd{
            PITChannel::Channel0,
            PITAccessMode::LobyteHibyte,
            PITOperatingMode::Mode3,
            PITBCDMode::Binary};

        printf("PIT: Setting period to %u ns, divisor: %u, cmd:%u \n", period_ns, divisor, cmd.toByte());

        // Send control word to the PIT
        IOPort::outportb(PIT_COMMAND_PORT, cmd.toByte());

        // Send divisor to the PIT
        IOPort::outportb(PIT_CHANNEL_0, static_cast<uint8_t>(divisor & 0xFF));        // Lobyte
        IOPort::outportb(PIT_CHANNEL_0, static_cast<uint8_t>((divisor >> 8) & 0xFF)); // Hibyte

        Handlers::AddInterruptHandler(32, pitHandler, nullptr);
        PIC::irq_enable(PIC::IRQ::Timer); // Enable PIT interrupt
    }

} // namespace PIT