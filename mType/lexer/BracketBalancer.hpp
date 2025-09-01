#pragma once
#include <stack>
#include <string>
#include "../errors/SourceLocation.hpp"

namespace lexer
{
    /**
     * Handles bracket and brace balance validation during parsing
     * Responsibility: Track opening/closing brackets and validate proper nesting
     */
    class BracketBalancer
    {
    private:
        std::stack<char> balanceStack;

    public:
        BracketBalancer() = default;

        // Balance operations
        void pushOpening(char bracket);
        void validateClosing(char bracket, const errors::SourceLocation& location);
        
        // State queries
        bool isEmpty() const { return balanceStack.empty(); }
        size_t getDepth() const { return balanceStack.size(); }
        char getTopOpening() const;
        
        // Validation
        bool isOpeningBracket(char c) const;
        bool isClosingBracket(char c) const;
        char getMatchingClosing(char opening) const;
        char getMatchingOpening(char closing) const;
        
        // State management
        void clear();
        std::stack<char> copyStack() const; // For peekNextToken functionality

    private:
        [[noreturn]] void throwBracketError(const std::string& message, const errors::SourceLocation& location);
    };
}