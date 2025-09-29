#include "ParsePatterns.hpp"

namespace parser::utilities
{
    bool ParsePatterns::isStatementKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::FOR:
        case TokenType::DO:
        case TokenType::SWITCH:
        case TokenType::BREAK:
        case TokenType::CONTINUE:
        case TokenType::RETURN:
        case TokenType::INT:
        case TokenType::FLOAT:
        case TokenType::STRING_TYPE:
        case TokenType::BOOL:
        case TokenType::VOID:
        case TokenType::FINAL:
        case TokenType::STATIC:
        case TokenType::FUNCTION:
        case TokenType::IMPORT:
        case TokenType::NATIVE:
            return true;
        default:
            return false;
        }
    }

    bool ParsePatterns::isExpressionStart(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::IDENTIFIER:
        case TokenType::INT_NUMBER:
        case TokenType::FLOAT_NUMBER:
        case TokenType::STRING_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::NULL_LITERAL:
        case TokenType::LPAREN:
        case TokenType::LBRACKET:
        case TokenType::NEW:
        case TokenType::MINUS:
        case TokenType::PLUS:
        case TokenType::NOT:
        case TokenType::INCREMENT:
        case TokenType::DECREMENT:
            return true;
        default:
            return false;
        }
    }

    bool ParsePatterns::isTypeKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::INT:
        case TokenType::FLOAT:
        case TokenType::STRING_TYPE:
        case TokenType::BOOL:
        case TokenType::VOID:
            return true;
        default:
            return false;
        }
    }

    void ParsePatterns::skipToMatchingBrace(TokenStream& stream)
    {
        int braceCount = 0;
        while (!stream.isAtEnd())
        {
            if (stream.check(TokenType::LBRACE))
            {
                braceCount++;
            }
            else if (stream.check(TokenType::RBRACE))
            {
                if (braceCount == 0)
                {
                    break;
                }
                braceCount--;
            }
            stream.advance();
        }
    }

    void ParsePatterns::skipToSemicolon(TokenStream& stream)
    {
        while (!stream.isAtEnd() && !stream.check(TokenType::SEMICOLON))
        {
            stream.advance();
        }
        if (stream.check(TokenType::SEMICOLON))
        {
            stream.advance();
        }
    }
}