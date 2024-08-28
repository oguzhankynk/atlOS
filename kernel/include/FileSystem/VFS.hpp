/**
 * @file VFS.hpp
 * @author Oguzhan KAYNAK
 * @brief
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software *without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit *persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR *PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR *OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F
#define D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F

#include <stdint.h>

namespace VFS
{

enum FS_Types
{
    FS_FILE,
    FS_DIRECTORY,
    FS_INVALID
};

struct FILE 
{

	char        name[32];
	uint32_t    flags;
	uint32_t    fileLength;
	uint32_t    id;
	uint32_t    eof;
	uint32_t    position;
	uint32_t    currentCluster;
	uint32_t    device;

};

using PFILE = FILE*;

struct FILESYSTEM
{

	char Name [8];
	FILE               (*Directory)  (const char* DirectoryName);
	void	           (*Mount)      ();
	void               (*Read)       (PFILE file, unsigned char* Buffer, unsigned int Length);
	void	           (*Close)      (PFILE);
	FILE               (*Open)       (const char* FileName);

};

using PFILESYSTEM = FILESYSTEM*;

FILE volOpenFile (const char* fname);
void volReadFile (PFILE file, unsigned char* Buffer, unsigned int Length);
void volCloseFile (PFILE file);
void volRegisterFileSystem (PFILESYSTEM, unsigned int deviceID);
void volUnregisterFileSystem (PFILESYSTEM);
void volUnregisterFileSystemByID (unsigned int deviceID);

} //namespace VFS

#endif /* D8146028_ADEC_4A31_A0B3_BE8F58ED3E8F */
