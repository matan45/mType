#include "TokenFactory.hpp"
#include "../value/StringPool.hpp"

namespace lexer
{
    Token TokenFactory::createToken(TokenType type, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{type, 0.0f, 0, pool.intern(""), location};
    }

    Token TokenFactory::createToken(TokenType type, std::string_view symbol, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{type, 0.0f, 0, pool.intern(symbolToString(symbol)), location};
    }

    Token TokenFactory::createIntegerToken(int value, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::INT_NUMBER, 0.0f, value, pool.intern(""), location};
    }

    Token TokenFactory::createFloatToken(float value, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::FLOAT_NUMBER, value, 0, pool.intern(""), location};
    }

    Token TokenFactory::createStringToken(const std::string& value, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::STRING_LITERAL, 0.0f, 0, pool.intern(value), location};
    }

    Token TokenFactory::createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::IDENTIFIER, 0.0f, 0, pool.intern(symbolToString(identifier)), location};
    }

    Token TokenFactory::createKeywordToken(TokenType keywordType, std::string_view keyword, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{keywordType, 0.0f, 0, pool.intern(symbolToString(keyword)), location};
    }

    Token TokenFactory::createOperatorToken(TokenType operatorType, std::string_view symbol, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{operatorType, 0.0f, 0, pool.intern(symbolToString(symbol)), location};
    }

    Token TokenFactory::createEndToken(const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::END, 0.0f, 0, pool.intern(""), location};
    }

    Token TokenFactory::createBooleanToken(bool value, const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        TokenType type = value ? TokenType::TRUE : TokenType::FALSE;
        std::string str = value ? "true" : "false";
        return Token{type, 0.0f, 0, pool.intern(str), location};
    }

    Token TokenFactory::createNullToken(const errors::SourceLocation& location)
    {
        auto& pool = value::StringPool::getInstance();
        return Token{TokenType::NULL_LITERAL, 0.0f, 0, pool.intern("null"), location};
    }

    std::string TokenFactory::symbolToString(std::string_view symbol)
    {
        return std::string(symbol);
    }
}