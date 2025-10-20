#include "BracketBalancer.hpp"
#include "../errors/ParseException.hpp"
#include <sstream>

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
            std::ostringstream oss;
            oss << "Unmatched closing ";
            if (bracket == ')') oss << "parenthesis";
            else if (bracket == ']') oss << "bracket";
            else oss << "brace";
            throwBracketError(oss.str(), location);
        }

        char expected = getMatchingClosing(balanceStack.top());
        if (bracket != expected)
        {
            std::ostringstream oss;
            oss << "Mismatched bracket: expected '" << expected
                << "' but found '" << bracket << "'";
            throwBracketError(oss.str(), location);
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
        return c == '(' || c == '{' || c == '[';
    }

    bool BracketBalancer::isClosingBracket(char c) const
    {
        return c == ')' || c == '}' || c == ']';
    }

    char BracketBalancer::getMatchingClosing(char opening) const
    {
        switch (opening)
        {
            case '(': return ')';
            case '{': return '}';
            case '[': return ']';
            default: return '\0';
        }
    }

    void BracketBalancer::clear()
    {
        balanceStack = std::stack<char>{}; // Idiomatic clear using assignment
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