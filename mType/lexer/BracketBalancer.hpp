#pragma once
#include <stack>
#include <cstddef>
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

        // Validation
        bool isOpeningBracket(char c) const;
        bool isClosingBracket(char c) const;

        // State management
        void clear();
        std::stack<char> copyStack() const; // For peekNextToken functionality

    private:
        /**
         * Gets the matching closing bracket for an opening bracket
         * @param opening Opening bracket character ('(', '{', '[')
         * @return Matching closing bracket (')', '}', ']')
         * @pre isOpeningBracket(opening) must be true
         */
        char getMatchingClosing(char opening) const;

        /**
         * Gets the top opening bracket from the stack
         * @return Opening bracket character at top of stack
         * @pre !isEmpty() - stack must not be empty
         */
        char getTopOpening() const;

        [[noreturn]] void throwBracketError(const std::string& message, const errors::SourceLocation& location);
    };
}
