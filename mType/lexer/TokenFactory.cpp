#include "TokenFactory.hpp"

namespace lexer
{
    Token TokenFactory::createToken(TokenType type, const errors::SourceLocation& location)
    {
        return Token{type, 0.0f, 0, "", location};
    }

    Token TokenFactory::createToken(TokenType type, std::string_view symbol, const errors::SourceLocation& location)
    {
        return Token{type, 0.0f, 0, symbolToString(symbol), location};
    }

    Token TokenFactory::createIntegerToken(int value, const errors::SourceLocation& location)
    {
        return Token{TokenType::INT_NUMBER, 0.0f, value, "", location};
    }

    Token TokenFactory::createFloatToken(float value, const errors::SourceLocation& location)
    {
        return Token{TokenType::FLOAT_NUMBER, value, 0, "", location};
    }

    Token TokenFactory::createStringToken(const std::string& value, const errors::SourceLocation& location)
    {
        return Token{TokenType::STRING_LITERAL, 0.0f, 0, value, location};
    }

    Token TokenFactory::createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location)
    {
        return Token{TokenType::IDENTIFIER, 0.0f, 0, symbolToString(identifier), location};
    }

    Token TokenFactory::createKeywordToken(TokenType keywordType, std::string_view keyword, const errors::SourceLocation& location)
    {
        return Token{keywordType, 0.0f, 0, symbolToString(keyword), location};
    }

    Token TokenFactory::createOperatorToken(TokenType operatorType, std::string_view symbol, const errors::SourceLocation& location)
    {
        return Token{operatorType, 0.0f, 0, symbolToString(symbol), location};
    }

    Token TokenFactory::createEndToken(const errors::SourceLocation& location)
    {
        return Token{TokenType::END, 0.0f, 0, "", location};
    }

    Token TokenFactory::createBooleanToken(bool value, const errors::SourceLocation& location)
    {
        TokenType type = value ? TokenType::TRUE : TokenType::FALSE;
        std::string str = value ? "true" : "false";
        return Token{type, 0.0f, 0, str, location};
    }

    Token TokenFactory::createNullToken(const errors::SourceLocation& location)
    {
        return Token{TokenType::NULL_LITERAL, 0.0f, 0, "null", location};
    }

    std::string TokenFactory::symbolToString(std::string_view symbol)
    {
        return std::string(symbol);
    }
}