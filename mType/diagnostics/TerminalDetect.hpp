#pragma once
#include <cstdio>

namespace diagnostics
{
    /**
     * Platform helpers for deciding whether to emit ANSI color escapes.
     *
     * Color decision (matches the no-color.org / clang / rustc convention):
     *   1. If --no-color is set, never emit color.
     *   2. If --color=always is set, always emit color.
     *   3. If $NO_COLOR is set (any value), never emit color.
     *   4. Otherwise, emit color iff the target stream is a terminal.
     */
    class TerminalDetect
    {
    public:
        /**
         * True if the given C FILE* refers to a terminal device.
         * Wraps `_isatty(_fileno(...))` on Windows and `isatty(fileno(...))`
         * on POSIX.
         */
        static bool isTerminal(std::FILE* stream);

        /**
         * True if the NO_COLOR environment variable is set to any value.
         */
        static bool noColorRequested();

        /**
         * On Windows, attempts to enable ENABLE_VIRTUAL_TERMINAL_PROCESSING
         * on the given stream once per process. Safe to call repeatedly;
         * subsequent calls are no-ops. Returns true if VT processing is
         * enabled (or if not on Windows).
         */
        static bool enableVirtualTerminalProcessing(std::FILE* stream);
    };
}
