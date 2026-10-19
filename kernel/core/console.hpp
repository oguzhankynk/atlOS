/**
 * @file console.hpp
 * @brief Where printf (and so logMsg and kernelPanic) writes to.
 */

#pragma once

namespace Drivers {
class CharacterSink;
}

namespace Core {

/**
 * @brief Send printf output to sink from now on.
 *
 * libc's putchar calls a plain C function and can not be given an object; this
 * is that boundary. kernelMain binds the VGA console first and switches to the
 * serial port once it is up.
 *
 * @return The previous sink (nullptr if there was none), so it can be restored.
 */
Drivers::CharacterSink* setConsole(Drivers::CharacterSink& sink);

} // namespace Core
