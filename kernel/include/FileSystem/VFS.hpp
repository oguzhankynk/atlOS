/**
 * @File VFS.hpp
 * @author Oguzhan KAYNAK
 * @brief Astraction layer for File systems
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

#ifndef D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F
#define D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F

#include <stdint.h>

#include <FileSystem/FileSystem.hpp>

namespace FileSystem
{
	namespace VFS
	{

		File *open(const char *filename);
		ssize_t read(File *File, void *buffer, size_t size);
		ssize_t write(File *File, const void *buffer, size_t size);
		int close(File *File);
		int remove(const char *filename);

		bool AddFileSystem(IFileSystem &fs);

	} // namespace VFS
} // namespace FileSystem

#endif /* D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F */
