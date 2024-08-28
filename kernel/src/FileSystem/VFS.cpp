/**
 * @file VFS.cpp
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

#include <FileSystem/VFS.hpp>

namespace VFS
{

static constexpr uint16_t DEVICE_MAX = 3;

//! File system list
PFILESYSTEM _FileSystems[DEVICE_MAX];

FILE volOpenFile (const char* fname) 
{

	if (fname) 
    {

		//! default to device 'a'
		unsigned char device = 'a';

		//! filename
		const char* filename = fname;

		//! in all cases, if fname[1]==':' then the first character must be device letter
		if (fname[1]==':') 
        {
			device = fname[0];
			filename += 2; //strip it from pathname
		}

		//! call filesystem
		if (_FileSystems [device - 'a']) 
        {
			//! set volume specific information and return file
			FILE file = _FileSystems[device - 'a']->Open (filename);
			file.device = device;
			return file;
		}
	}

	FILE file;
	file.flags = FS_INVALID;
	return file;
}

void volRegisterFileSystem (PFILESYSTEM fsys, unsigned int deviceID) 
{

	if (deviceID < DEVICE_MAX)
		if (fsys)
			_FileSystems[ deviceID ] = fsys;
}

}//namespace VFS