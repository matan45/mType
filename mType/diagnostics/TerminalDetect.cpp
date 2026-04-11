#include "TerminalDetect.hpp"
#include <cstdlib>

#if defined(_WIN32)
    #include <io.h>
    #include <windows.h>
    #define MTYPE_ISATTY(fd)  _isatty(fd)
    #define MTYPE_FILENO(fp)  _fileno(fp)
#else
    #include <unistd.h>
    #define MTYPE_ISATTY(fd)  isatty(fd)
    #define MTYPE_FILENO(fp)  fileno(fp)
#endif

namespace diagnostics
{
    bool TerminalDetect::isTerminal(std::FILE* stream)
    {
        if (stream == nullptr)
        {
            return false;
        }
        return MTYPE_ISATTY(MTYPE_FILENO(stream)) != 0;
    }

    bool TerminalDetect::noColorRequested()
    {
        const char* value = std::getenv("NO_COLOR");
        return value != nullptr && value[0] != '\0';
    }

    bool TerminalDetect::enableVirtualTerminalProcessing(std::FILE* stream)
    {
#if defined(_WIN32)
        if (stream == nullptr)
        {
            return false;
        }
        // Map the FILE* to a Windows HANDLE.
        HANDLE handle = INVALID_HANDLE_VALUE;
        if (stream == stdout)
        {
            handle = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        else if (stream == stderr)
        {
            handle = GetStdHandle(STD_ERROR_HANDLE);
        }
        else
        {
            return false;
        }
        if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
        {
            return false;
        }
        DWORD mode = 0;
        if (!GetConsoleMode(handle, &mode))
        {
            // Not a real console (probably redirected) — caller should
            // already have decided not to emit color via isTerminal().
            return false;
        }
        const DWORD desired = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (mode == desired)
        {
            return true;
        }
        return SetConsoleMode(handle, desired) != 0;
#else
        (void)stream;
        return true;
#endif
    }
}
