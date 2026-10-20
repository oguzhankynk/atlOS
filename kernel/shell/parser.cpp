/**
 * @file parser.cpp
 * @author Oguzhan KAYNAK
 * @brief Splitting a command line into words, and numbers in the words.
 */

#include <shell/parser.hpp>

namespace Shell {

static bool isSpace(char c)
{
    return c == ' ' || c == '\t';
}

int splitWords(char* line, char* words[], int maxWords)
{
    int count = 0;
    char* in = line;

    while (true) {
        while (isSpace(*in))
            in++;
        if (!*in)
            return count;
        if (count == maxWords)
            return -1;

        // The word is written over the line: quotes take no space in the result
        char* out = in;
        words[count++] = out;
        bool quoted = false;
        while (*in && (quoted || !isSpace(*in))) {
            if (*in == '"')
                quoted = !quoted;
            else
                *out++ = *in;
            in++;
        }
        if (quoted)
            return -1;

        bool end = !*in;
        *out = '\0';
        if (end)
            return count;
        in++;
    }
}

bool parseNumber(const char* text, uint64_t& value)
{
    uint64_t base = 10;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        text += 2;
    }
    if (!*text)
        return false;

    uint64_t result = 0;
    for (; *text; text++) {
        uint64_t digit;
        char c = *text;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else if (base == 16 && c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else
            return false;

        if (result > (UINT64_MAX - digit) / base)
            return false;
        result = result * base + digit;
    }
    value = result;
    return true;
}

} // namespace Shell
