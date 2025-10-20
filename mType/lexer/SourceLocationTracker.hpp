#pragma once
#include <string>
#include <vector>
#include "../errors/SourceLocation.hpp"

namespace lexer
{
    /**
     * Handles line and column tracking during lexical analysis
     * Responsibility: Track position in source code and provide location information
     */
    class SourceLocationTracker
    {
    private:
        std::string filename;
        int currentLine;
        int currentColumn;
        std::vector<std::string> lines;

    public:
        explicit SourceLocationTracker(const std::string& filename);

        // Position tracking
        void advance(char current);
        void setPosition(int line, int column);
        
        // Location queries
        errors::SourceLocation getCurrentLocation() const;
        int getCurrentLine() const { return currentLine; }
        int getCurrentColumn() const { return currentColumn; }
        
        // Line management
        void splitIntoLines(const std::string& input);
    };
}