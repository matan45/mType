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
        
        // Handle case where input doesn't end with newline
        if (!input.empty() && input.back() != '\n')
        {
            // The last line is already added by getline
        }
    }

    std::string SourceLocationTracker::getLineContent(int lineNumber) const
    {
        if (lineNumber >= 1 && static_cast<size_t>(lineNumber) <= lines.size())
        {
            return lines[lineNumber - 1]; // Convert to 0-based index
        }
        return "";
    }

    void SourceLocationTracker::reset()
    {
        currentLine = 1;
        currentColumn = 1;
        lines.clear();
    }
}