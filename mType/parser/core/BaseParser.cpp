#include "BaseParser.hpp"
#include "../../exceptions/DomainExceptions.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::core
{
    void BaseParser::expectToken(TokenType type, const std::string& parserContext)
    {
        if (!tokenStream.check(type))
        {
            std::string expectedName = "TOKEN"; // Simplified for now
            std::string actualName = "TOKEN"; // Simplified for now
            std::string message = "Expected token but found different token";

            ErrorContext error(getCurrentLocation(), message, ErrorSeverity::Error, parserContext);
            errorHandler->reportError(error);

            throw errors::ParseException(message);
        }
        tokenStream.advance();
    }

    bool BaseParser::tryConsumeToken(TokenType type) noexcept
    {
        if (tokenStream.check(type))
        {
            tokenStream.advance();
            return true;
        }
        return false;
    }

    SourceLocation BaseParser::getCurrentLocation() const noexcept
    {
        return tokenStream.location();
    }

    void BaseParser::reportError(const std::string& message, const std::string& parserContext) const
    {
        ErrorContext error(getCurrentLocation(), message, ErrorSeverity::Error, parserContext);
        errorHandler->reportError(error);
    }

    void BaseParser::reportWarning(const std::string& message, const std::string& parserContext) const
    {
        errorHandler->reportWarning(message, getCurrentLocation(), parserContext);
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
}