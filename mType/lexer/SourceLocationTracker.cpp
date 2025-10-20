#include "SourceLocationTracker.hpp"
#include <sstream>

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
        // Validate inputs - line and column should be positive
        if (line < 1 || column < 1)
        {
            return; // Silently ignore invalid positions
        }
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