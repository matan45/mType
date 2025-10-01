#include "BaseParser.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::core
{
    BaseParser::BaseParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
    }

    void BaseParser::expectToken(TokenType type)
    {
        if (!tokenStream.check(type))
        {
            std::string expectedName = "TOKEN"; // Simplified for now
            std::string actualName = "TOKEN"; // Simplified for now
            std::string message = "Expected token but found different token";

            throw ParseException(message, tokenStream.current().location);
        }
        tokenStream.advance();
    }

    bool BaseParser::tryConsumeToken(TokenType type)
    {
        if (tokenStream.check(type))
        {
            tokenStream.advance();
            return true;
        }
        return false;
    }


    void BaseParser::recoverToToken(TokenType type)
    {
        while (!isAtEnd() && !tokenStream.check(type))
        {
            tokenStream.advance();
        }
    }

    void BaseParser::recoverToStatement()
    {
        while (!isAtEnd())
        {
            if (tokenStream.check(TokenType::SEMICOLON) ||
                tokenStream.check(TokenType::LBRACE) ||
                tokenStream.check(TokenType::RBRACE))
            {
                break;
            }
            tokenStream.advance();
        }
    }

    void BaseParser::recoverToBlock()
    {
        int braceCount = 0;
        while (!isAtEnd())
        {
            if (tokenStream.check(TokenType::LBRACE))
            {
                braceCount++;
            }
            else if (tokenStream.check(TokenType::RBRACE))
            {
                if (braceCount == 0)
                {
                    break;
                }
                braceCount--;
            }
            tokenStream.advance();
        }
    }

    bool BaseParser::isAtEnd() const
    {
        return tokenStream.isAtEnd();
    }

    const Token& BaseParser::currentToken() const
    {
        return tokenStream.current();
    }
}
