/**
 * @file line_editor.cpp
 * @author Oguzhan KAYNAK
 * @brief Shell::LineEditor: typed characters to a command line.
 */

#include <shell/line_editor.hpp>

#include <stdio.h>
#include <string.h>

namespace Shell {

static constexpr char ctrlC = 3;
static constexpr char escape = 0x1B;
static constexpr char del = 0x7F;

void LineEditor::start()
{
    m_length = 0;
    m_line[0] = '\0';
    m_browse = 0;
    m_escape = Escape::None;
}

void LineEditor::replaceLine(const char* text)
{
    // Erase what is shown: back, overwrite with a space, back again
    for (uint32_t i = 0; i < m_length; i++)
        printf("\b \b");

    strncpy(m_line, text, maxLength);
    m_line[maxLength] = '\0';
    m_length = strlen(m_line);
    printf("%s", m_line);
}

void LineEditor::browseHistory(int direction)
{
    // Up (+1) goes to older lines, down (-1) back to newer ones and finally to an empty line
    uint32_t browse = m_browse + direction;
    if (direction > 0 && browse > m_historyCount)
        return;
    if (direction < 0 && !m_browse)
        return;

    m_browse = browse;
    replaceLine(m_browse ? m_history[m_historyCount - m_browse] : "");
}

LineEditor::Result LineEditor::feed(char c)
{
    // Escape sequences: ESC [ then parameters, ended by a byte 0x40..0x7E
    if (m_escape == Escape::Started) {
        m_escape = c == '[' ? Escape::Bracket : Escape::None;
        return Result::Continue;
    }
    if (m_escape == Escape::Bracket) {
        if (c < 0x40 || c > 0x7E)
            return Result::Continue;
        m_escape = Escape::None;
        if (c == 'A')
            browseHistory(+1);
        else if (c == 'B')
            browseHistory(-1);
        return Result::Continue;
    }

    // A terminal may send "\r\n" for Enter: one line, not two
    bool afterCarriageReturn = m_lastWasCarriageReturn;
    m_lastWasCarriageReturn = c == '\r';
    if (c == '\n' && afterCarriageReturn)
        return Result::Continue;

    switch (c) {
    case escape:
        m_escape = Escape::Started;
        return Result::Continue;

    case '\r':
    case '\n':
        printf("\n");
        if (m_length && (!m_historyCount || strcmp(m_history[m_historyCount - 1], m_line) != 0)) {
            // Full: drop the oldest
            if (m_historyCount == historySize) {
                memmove(m_history[0], m_history[1], sizeof(m_history[0]) * (historySize - 1));
                m_historyCount--;
            }
            memcpy(m_history[m_historyCount++], m_line, m_length + 1);
        }
        return Result::Done;

    case ctrlC:
        printf("^C\n");
        start();
        return Result::Cancelled;

    case '\b':
    case del:
        if (m_length) {
            m_line[--m_length] = '\0';
            printf("\b \b");
        }
        return Result::Continue;

    default:
        if (c >= ' ' && c < del && m_length < maxLength) {
            m_line[m_length++] = c;
            m_line[m_length] = '\0';
            printf("%c", c);
        }
        return Result::Continue;
    }
}

} // namespace Shell
