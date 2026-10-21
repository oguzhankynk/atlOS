/**
 * @file interpreter.hpp
 * @author Oguzhan KAYNAK
 * @brief Shell::Interpreter: the kernel shell. Reads lines, splits them into
 * words and runs the command named by the first one.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <shell/line_editor.hpp>

namespace Drivers {
class CharacterSource;
class SerialPort;
class VGATextConsole;
}

namespace Fs {
class Vfs;
struct BlockDeviceList;
}

namespace Memory {
class AddressSpace;
class PhysicalMemoryManager;
}

namespace Threading {
class Scheduler;
}

namespace Process {
class Runtime;
}

namespace Shell {

/// What the commands work on, handed in by kernelMain (no globals).
struct Context {
    Memory::PhysicalMemoryManager& pmm;
    Memory::AddressSpace& space;
    Fs::Vfs& vfs;
    Fs::BlockDeviceList& disks; ///< the disk command reads them
    Process::Runtime& runtime;
    Drivers::VGATextConsole* screen; ///< for clear, may be null
    Drivers::SerialPort* serial; ///< for clear, may be null
};

class Interpreter {
public:
    static constexpr int maxWords = 16;
    static constexpr size_t maxPath = 128;

    /// Lines come from input; output goes through printf (the console).
    Interpreter(Drivers::CharacterSource& input, Context& context);

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

    /// Prompt, read a line, run it; forever.
    [[noreturn]] void run();

    /// Run one command line (changed in place). The self tests call this.
    void execute(char* line);

    /**
     * @brief Make path absolute and clean: relative to the current directory,
     * "." and ".." resolved, no repeated or trailing "/".
     * @return false if the result does not fit into size.
     */
    bool resolvePath(const char* path, char* out, size_t size) const;

    const char* currentDirectory() const { return m_cwd; }

    /// Change the current directory; false if path is not a directory.
    bool changeDirectory(const char* path);

    Context& context() { return m_context; }

private:
    Drivers::CharacterSource& m_input;
    Context& m_context;
    LineEditor m_editor;
    char m_cwd[maxPath] { "/" };
};

/// One entry of the command table (commands.cpp).
struct Command {
    const char* group; ///< help lists the commands by group
    const char* name;
    const char* usage;
    const char* help;
    void (*run)(Interpreter& shell, int argc, char** argv);
};

/// The command table, ended by an entry with name nullptr.
const Command* commands();

} // namespace Shell
