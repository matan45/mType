#include "Lexer.hpp"

namespace lexer
{
    Lexer::Lexer(const std::string& input, const std::string& fname)
    {
    }

    Token Lexer::getNextToken()
    {
        return Token();
    }

    Token Lexer::peekNextToken()
    {
        return Token();
    }

    void Lexer::splitIntoLines()
    {
    }

    float Lexer::parseFloat()
    {
        return 0;
    }

    int Lexer::parseInteger()
    {
        return 0;
    }

    std::string Lexer::parseIdentifier()
    {
        return "";
    }

    std::string Lexer::parseStringLiteral()
    {
        return "";
    }

    void Lexer::skipWhitespaceAndComments()
    {
    }

    void Lexer::advance()
    {
    }

    void Lexer::throwError(const std::string& message)
    {
    }
}
