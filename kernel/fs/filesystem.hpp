/**
 * @File FileSystem.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Interface class definations of File systems
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
 */

#pragma once

#include <stdint.h>

class Disk;

namespace FileSystem {
class IFileSystem;
struct File;

struct File {
    IFileSystem* owner {};
    size_t position {};
    size_t size {};
    bool isOpen {};
};

class IFileSystem {
protected:
    const Disk& m_device;
    uint32_t m_lbaFirstSector;

public:
    IFileSystem(const Disk& device, uint32_t lbaFirsSector) noexcept
        : m_device { device }
        , m_lbaFirstSector { lbaFirsSector }
    {
    }
    virtual ~IFileSystem() noexcept = default;

    virtual bool mount() = 0;
    virtual File* open(const char* filename) = 0;
    virtual ssize_t read(File* file, void* buffer, size_t size) = 0;
    virtual ssize_t write(File* file, const void* buffer, size_t size) = 0;
    virtual int close(File* file) = 0;
    virtual int remove(const char* filename) = 0;
    virtual void printFilelist() = 0;
    virtual const Disk& getDisk() const
    {
        return m_device;
    }
};

} // namespace FileSystem
