/**
 * @file strcmp.cpp
 * @author Oguzhan KAYNAK ()
 * @brief 
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
 * 
 */
 

#include <string.h>
int strcmp(const char* str1, const char* str2)
{
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

int strncmp(const char* str1, const char* str2, size_t num)
{
    for(auto i=0; i<num; i++)
    {
        if(*(str1+i) != *(str2+i) || *(str1+i) == '\0' || *(str2+i) == '\0')
        {
            return ( *(unsigned char *)(str1+i) - *(unsigned char *)(str2+i) );
        }
    }

    return 0;

}

char* strcpy(char* des, const char* src)
{
    char* dest_start = des;
    while (*src != '\0') {
        *des = *src;
        des++;
        src++;
    }
    *des = '\0';  
    return dest_start;
}

char* strncpy(char *des, const char *src, size_t num )
{
    char* dest_start = des;
    size_t i;

    for (i = 0; i < num && src[i] != '\0'; i++) {
        des[i] = src[i];
    }
    
    for ( ; i < num; i++) {
        des[i] = '\0';
    }

    return dest_start;
}