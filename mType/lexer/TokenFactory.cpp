#include "TokenFactory.hpp"
#include <cstdint>

namespace lexer
{
    Token TokenFactory::createIntegerToken(int64_t value, const errors::SourceLocation& location)
    {
        return Token{TokenType::INT_NUMBER, 0.0, value, std::string_view{}, 0u, location};
    }

    Token TokenFactory::createFloatToken(double value, const errors::SourceLocation& location)
    {
        return Token{TokenType::FLOAT_NUMBER, value, 0, std::string_view{}, 0u, location};
    }

    Token TokenFactory::createStringToken(std::string_view value, const errors::SourceLocation& location)
    {
        return Token{TokenType::STRING_LITERAL, 0.0, 0, value,
                     static_cast<uint32_t>(value.size()), location};
    }

    Token TokenFactory::createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location)
    {
        return Token{TokenType::IDENTIFIER, 0.0, 0, identifier,
                     static_cast<uint32_t>(identifier.size()), location};
    }

    Token TokenFactory::createKeywordToken(TokenType keywordType, std::string_view keyword,
                                           const errors::SourceLocation& location)
    {
        return Token{keywordType, 0.0, 0, keyword,
                     static_cast<uint32_t>(keyword.size()), location};
    }

    Token TokenFactory::createOperatorToken(TokenType operatorType, std::string_view symbol,
                                            const errors::SourceLocation& location)
    {
        return Token{operatorType, 0.0, 0, symbol,
                     static_cast<uint32_t>(symbol.size()), location};
    }

    Token TokenFactory::createEndToken(const errors::SourceLocation& location)
    {
        return Token{TokenType::END, 0.0, 0, std::string_view{}, 0u, location};
    }
}
