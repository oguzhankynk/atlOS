/**
 * @file vfs.cpp
 * @author Oguzhan KAYNAK
 * @brief Astraction layer for file systems
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

#include <fs/vfs.hpp>

#include <stdio.h>
#include <string.h>

#include <dev/device.hpp>
#include <drivers/disk/disk.hpp>

namespace FileSystem {
namespace VFS {
    struct FileSystemInfo {
        char name[32] {};
        IFileSystem* fs {};
    };

    static constexpr uint16_t maxFileSystem = 5;

    static FileSystemInfo* s_fileSystems[maxFileSystem] {};

    bool addFileSystem(IFileSystem& fs)
    {
        int fsIndex = 0;

        for (fsIndex = 0; fsIndex < maxFileSystem; fsIndex++) {
            if (s_fileSystems[fsIndex] == nullptr) {
                break;
            }
        }

        if (fsIndex == maxFileSystem) {
            return false;
        }

        auto& newFs = s_fileSystems[fsIndex];

        newFs = new FileSystemInfo;

        if (newFs == nullptr) {
            return false;
        }

        newFs->fs = &fs;
        snprintf(newFs->name, sizeof(newFs->name), "/fs%d", fsIndex);

        auto ret = newFs->fs->mount();

        if (ret == false) {
            delete newFs;
            newFs = nullptr;
            return false;
        }

        printf("vfs: %s mounted on %s \n", newFs->name, newFs->fs->getDisk().getName());

        newFs->fs->printFilelist();

        return true;
    }
} // namespace VFS
} // namespace FileSystem