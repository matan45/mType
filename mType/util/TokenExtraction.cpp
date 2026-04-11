#include "TokenExtraction.hpp"

#include <cctype>

namespace util
{
    namespace
    {
        bool isIdentByte(char c)
        {
            return (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_';
        }
    }

    std::string extractIdentifierTokenBefore(std::string_view line, int column)
    {
        if (line.empty() || column <= 0)
        {
            return {};
        }

        const int clamped = (column > static_cast<int>(line.size()))
            ? static_cast<int>(line.size())
            : column;

        // The cursor sits at `clamped`; the character immediately to
        // its left is the last byte of the token (if any).
        int end = clamped;
        if (end <= 0 || !isIdentByte(line[end - 1]))
        {
            return {};
        }

        int start = end;
        while (start > 0 && isIdentByte(line[start - 1]))
        {
            --start;
        }
        return std::string(line.substr(start, end - start));
    }
}
