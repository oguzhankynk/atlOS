/**
 * @file ATA.hpp
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

#ifndef D2A30C7E_444F_4393_BCBF_81177EB71649
#define D2A30C7E_444F_4393_BCBF_81177EB71649

#include <stdint.h>

namespace ATA
{
    void ata_read_sector(uint32_t lba, uint8_t *buffer);
    void ata_write_sector(uint32_t lba, uint8_t *buffer);
    
};//namespace ATA
#endif /* D2A30C7E_444F_4393_BCBF_81177EB71649 */
