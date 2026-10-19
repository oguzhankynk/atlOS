/**
 * @file VFS.cpp
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

#include <FileSystem/VFS.hpp>

#include <string.h>
#include <stdio.h>

#include <Device/Device.hpp>
#include <Interfaces/Disk/Disk.hpp>

namespace FileSystem
{
	namespace VFS
	{
		struct FileSystemInfo
		{
			char name[32]{};
			IFileSystem *fs{};
		};

		static constexpr uint16_t MAX_FILE_SYSTEM = 5;

		static FileSystemInfo *fileSystems[MAX_FILE_SYSTEM]{};

		bool AddFileSystem(IFileSystem &fs)
		{
			int fsIndex = 0;

			for (fsIndex = 0; fsIndex < MAX_FILE_SYSTEM; fsIndex++)
			{
				if (fileSystems[fsIndex] == nullptr)
				{
					break;
				}
			}

			if (fsIndex == MAX_FILE_SYSTEM)
			{
				return false;
			}

			auto &newFs = fileSystems[fsIndex];

			newFs = new FileSystemInfo;

			if (newFs == nullptr)
			{
				return false;
			}

			newFs->fs = &fs;
			snprintf(newFs->name, sizeof(newFs->name), "/fs%d", fsIndex);

			auto ret = newFs->fs->mount();

			if (ret == false)
			{
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