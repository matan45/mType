#include "BracketBalancer.hpp"
#include "../errors/ParseException.hpp"

namespace lexer
{
    void BracketBalancer::pushOpening(char bracket)
    {
        if (isOpeningBracket(bracket))
        {
            balanceStack.push(bracket);
        }
    }

    void BracketBalancer::validateClosing(char bracket, const errors::SourceLocation& location)
    {
        if (!isClosingBracket(bracket))
        {
            return; // Not a closing bracket, nothing to validate
        }

        if (balanceStack.empty())
        {
            std::string message = "Unmatched closing ";
            message += (bracket == ')' ? "parenthesis" : "brace");
            throwBracketError(message, location);
        }

        char expected = getMatchingClosing(balanceStack.top());
        if (bracket != expected)
        {
            std::string message = "Mismatched bracket: expected '";
            message += expected;
            message += "' but found '";
            message += bracket;
            message += "'";
            throwBracketError(message, location);
        }

        balanceStack.pop();
    }

    char BracketBalancer::getTopOpening() const
    {
        if (balanceStack.empty())
        {
            return '\0';
        }
        return balanceStack.top();
    }

    bool BracketBalancer::isOpeningBracket(char c) const
    {
        return c == '(' || c == '{';
    }

    bool BracketBalancer::isClosingBracket(char c) const
    {
        return c == ')' || c == '}';
    }

    char BracketBalancer::getMatchingClosing(char opening) const
    {
        switch (opening)
        {
            case '(': return ')';
            case '{': return '}';
            default: return '\0';
        }
    }

    char BracketBalancer::getMatchingOpening(char closing) const
    {
        switch (closing)
        {
            case ')': return '(';
            case '}': return '{';
            default: return '\0';
        }
    }

    void BracketBalancer::clear()
    {
        while (!balanceStack.empty())
        {
            balanceStack.pop();
        }
    }

    std::stack<char> BracketBalancer::copyStack() const
    {
        return balanceStack; // Copy constructor
    }

    void BracketBalancer::throwBracketError(const std::string& message, const errors::SourceLocation& location)
    {
        throw errors::ParseException(message, location);
    }
}