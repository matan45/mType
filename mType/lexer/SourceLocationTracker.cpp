#include "SourceLocationTracker.hpp"
#include <sstream>
#include <cassert>

namespace lexer
{
    SourceLocationTracker::SourceLocationTracker(const std::string& filename)
        : filename(filename), currentLine(1), currentColumn(1)
    {
    }

    void SourceLocationTracker::advance(char current)
    {
        if (current == '\n')
        {
            currentLine++;
            currentColumn = 1;
        }
        else
        {
            currentColumn++;
        }
    }

    void SourceLocationTracker::setPosition(int line, int column)
    {
        // Precondition: Source positions use 1-based indexing
        // Invalid values indicate a programming error in the caller
        assert(line >= 1 && "Line number must be >= 1 (1-based indexing)");
        assert(column >= 1 && "Column number must be >= 1 (1-based indexing)");

        currentLine = line;
        currentColumn = column;
    }

    errors::SourceLocation SourceLocationTracker::getCurrentLocation() const
    {
        return errors::SourceLocation(filename, currentLine, currentColumn);
    }

    void SourceLocationTracker::splitIntoLines(const std::string& input)
    {
        lines.clear();
        std::istringstream stream(input);
        std::string line;

        while (std::getline(stream, line))
        {
            lines.push_back(line);
        }
        // Note: std::getline handles input without trailing newline correctly
    }
}
