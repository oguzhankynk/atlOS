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
    for (char* p = des; (*p++ = *src++); );

    return des;
}