#include "BracketBalancer.hpp"
#include "../errors/ParseException.hpp"
#include <sstream>
#include <cassert>

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
        // Precondition: stack must not be empty
        // Callers must check isEmpty() before calling this method
        assert(!balanceStack.empty() &&
            "Precondition violated: cannot get top of empty bracket stack");

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
        // Precondition: opening must be a valid opening bracket
        // This is guaranteed by pushOpening() which filters invalid characters
        assert(isOpeningBracket(opening) &&
            "Precondition violated: opening must be a valid bracket ('(', '{', or '[')");

        switch (opening)
        {
        case '(': return ')';
        case '{': return '}';
        case '[': return ']';
        default:
            // Should be unreachable due to assertion and input validation
            assert(false && "Unreachable: all valid opening brackets are handled above");
            return '\0'; // Satisfy compiler warning
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
