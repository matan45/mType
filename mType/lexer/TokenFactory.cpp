#include "TokenFactory.hpp"
#include "../value/StringPool.hpp"

namespace lexer
{
    value::InternedString TokenFactory::internString(std::string_view str)
    {
        return value::StringPool::getInstance().intern(std::string(str));
    }

    value::InternedString TokenFactory::emptyString()
    {
        static value::InternedString empty = value::StringPool::getInstance().intern("");
        return empty;
    }

    Token TokenFactory::createIntegerToken(int64_t value, const errors::SourceLocation& location)
    {
        return Token{TokenType::INT_NUMBER, 0.0, value, emptyString(), location};
    }

    Token TokenFactory::createFloatToken(double value, const errors::SourceLocation& location)
    {
        return Token{TokenType::FLOAT_NUMBER, value, 0, emptyString(), location};
    }

    Token TokenFactory::createStringToken(std::string_view value, const errors::SourceLocation& location)
    {
        return Token{TokenType::STRING_LITERAL, 0.0, 0, internString(value), location};
    }

    Token TokenFactory::createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location)
    {
        return Token{TokenType::IDENTIFIER, 0.0, 0, internString(identifier), location};
    }

    Token TokenFactory::createKeywordToken(TokenType keywordType, std::string_view keyword,
                                           const errors::SourceLocation& location)
    {
        return Token{keywordType, 0.0, 0, internString(keyword), location};
    }

    Token TokenFactory::createOperatorToken(TokenType operatorType, std::string_view symbol,
                                            const errors::SourceLocation& location)
    {
        return Token{operatorType, 0.0, 0, internString(symbol), location};
    }

    Token TokenFactory::createEndToken(const errors::SourceLocation& location)
    {
        return Token{TokenType::END, 0.0, 0, emptyString(), location};
    }
}
