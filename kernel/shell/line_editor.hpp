/**
 * @file line_editor.hpp
 * @author Oguzhan KAYNAK
 * @brief Shell::LineEditor: turns typed characters into a command line, with
 * backspace, Ctrl+C and a history behind the up/down arrows.
 */

#pragma once

#include <stdint.h>

namespace Shell {

/**
 * @brief Characters go in one at a time (feed); the editor echoes them
 * through printf and says when a line is complete.
 *
 * Understood input, from the keyboard driver and from serial terminals alike:
 * printable characters, Enter ('\r' or '\n'), Backspace ('\b' or DEL 0x7f),
 * Ctrl+C (cancel the line), and the escape sequences ESC [ A / ESC [ B (up,
 * down: history). Other escape sequences are skipped.
 */
class LineEditor {
public:
    static constexpr uint32_t maxLength = 127;
    static constexpr uint32_t historySize = 16;

    enum class Result {
        Continue, ///< the line is not complete yet
        Done, ///< Enter: line() holds the line, it went into the history
        Cancelled, ///< Ctrl+C: the line was dropped
    };

    /// Start an empty line (after Done or Cancelled).
    void start();

    Result feed(char c);

    const char* line() const { return m_line; }

    uint32_t historyCount() const { return m_historyCount; }

private:
    void replaceLine(const char* text);
    void browseHistory(int direction);

    char m_line[maxLength + 1] {};
    uint32_t m_length {};

    enum class Escape {
        None,
        Started, // got ESC
        Bracket, // got ESC [
    };
    Escape m_escape { Escape::None };
    bool m_lastWasCarriageReturn {};

    // Oldest first; m_browse counts back from the newest (0: not browsing)
    char m_history[historySize][maxLength + 1] {};
    uint32_t m_historyCount {};
    uint32_t m_browse {};
};

} // namespace Shell
