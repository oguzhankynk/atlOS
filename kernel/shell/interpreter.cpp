/**
 * @file interpreter.cpp
 * @author Oguzhan KAYNAK
 * @brief Shell::Interpreter: the kernel shell.
 */

#include <shell/interpreter.hpp>

#include <stdio.h>
#include <string.h>

#include <core/banner.hpp>
#include <drivers/character_source.hpp>
#include <drivers/vga_console.hpp>
#include <fs/vfs.hpp>
#include <mm/pmm.hpp>
#include <proc/process.hpp>
#include <shell/parser.hpp>

namespace Shell {

Interpreter::Interpreter(Drivers::CharacterSource& input, Context& context)
    : m_input(input)
    , m_context(context)
{
}

static void printMount(const Fs::FileInfo& mount, void*)
{
    printf(" /%s", mount.name);
}

void Interpreter::run()
{
    // A clean screen with the banner; the serial log keeps the boot messages
    if (m_context.screen)
        m_context.screen->clear();
    atlOSBanner();

    Memory::PhysicalMemoryManager& pmm = m_context.pmm;
    printf("  memory: %lu MB free   disks:", pmm.freeBlockCount() * Memory::PhysicalMemoryManager::blockSize >> 20);
    if (!m_context.vfs.mountCount())
        printf(" none");
    m_context.vfs.listDirectory("/", printMount, nullptr);
    printf("\n  \"help\" lists the commands, \"run /disk0/BIN/HELLO.ELF\" starts a user program\n\n");

    while (true) {
        // Programs that exited since the last prompt give their memory back
        m_context.runtime.reap();
        printf("atlOS:%s> ", m_cwd);
        m_editor.start();

        LineEditor::Result result;
        do
            result = m_editor.feed(m_input.read()); // sleeps until a key arrives
        while (result == LineEditor::Result::Continue);

        if (result == LineEditor::Result::Done) {
            char line[LineEditor::maxLength + 1];
            memcpy(line, m_editor.line(), strlen(m_editor.line()) + 1);
            execute(line);
        }
    }
}

void Interpreter::execute(char* line)
{
    char* words[maxWords];
    int count = splitWords(line, words, maxWords);
    if (count < 0) {
        printf("missing \" or too many words (at most %d)\n", maxWords);
        return;
    }
    if (!count)
        return;

    for (const Command* command = commands(); command->name; command++) {
        if (!strcmp(command->name, words[0])) {
            command->run(*this, count, words);
            return;
        }
    }
    printf("%s: unknown command, \"help\" lists them\n", words[0]);
}

bool Interpreter::resolvePath(const char* path, char* out, size_t size) const
{
    // Start from the root (absolute path) or the current directory, then
    // apply the components one by one
    char result[maxPath];
    size_t length = 0;
    if (path[0] != '/') {
        length = strlen(m_cwd);
        memcpy(result, m_cwd, length + 1);
        if (length == 1)
            length = 0; // "/" : components are appended as "/name"
    }

    while (*path) {
        while (*path == '/')
            path++;
        size_t componentLength = 0;
        while (path[componentLength] && path[componentLength] != '/')
            componentLength++;
        if (!componentLength)
            break;

        if (componentLength == 1 && path[0] == '.') {
            // nothing
        } else if (componentLength == 2 && path[0] == '.' && path[1] == '.') {
            while (length && result[length - 1] != '/')
                length--;
            if (length)
                length--; // the "/" before the removed component
        } else {
            if (length + 1 + componentLength >= sizeof(result))
                return false;
            result[length++] = '/';
            memcpy(result + length, path, componentLength);
            length += componentLength;
        }
        path += componentLength;
    }

    if (!length)
        result[length++] = '/';
    result[length] = '\0';

    if (length >= size)
        return false;
    memcpy(out, result, length + 1);
    return true;
}

bool Interpreter::changeDirectory(const char* path)
{
    char resolved[maxPath];
    Fs::FileInfo info;
    if (!resolvePath(path, resolved, sizeof(resolved)) || !m_context.vfs.stat(resolved, info) || !info.isDirectory)
        return false;
    memcpy(m_cwd, resolved, strlen(resolved) + 1); // both maxPath long
    return true;
}

} // namespace Shell
