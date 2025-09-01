#include "TokenStream.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace errors;

    TokenStream::TokenStream(Lexer& lex) : lexer(lex)
    {
        // Initialize with first token
        advance();
    }

    void TokenStream::advance()
    {
        currentToken = lexer.getNextToken();
    }

    bool TokenStream::check(TokenType type) const noexcept
    {
        return currentToken.type == type;
    }

    bool TokenStream::isAtEnd() const noexcept
    {
        return currentToken.type == TokenType::END;
    }

    bool TokenStream::match(TokenType type)
    {
        if (check(type))
        {
            advance();
            return true;
        }
        return false;
    }

    void TokenStream::expect(TokenType type)
    {
        if (!match(type))
        {
            throw ParseException("Expected token type but found different type", 
                               currentToken.location);
        }
    }

    Token TokenStream::peek() const
    {
        return lexer.peekNextToken();
    }
}