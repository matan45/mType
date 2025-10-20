#include "StatementTypeDetector.hpp"

namespace parser::utilities
{
    StatementType StatementTypeDetector::analyzeAccessModifier(const TokenStream& stream)
    {
        Token next = stream.peek();
        if (next.type == TokenType::CLASS)
        {
            return StatementType::CLASS;
        }
        else if (next.type == TokenType::INTERFACE)
        {
            return StatementType::INTERFACE;
        }
        else if (next.type == TokenType::FUNCTION || next.type == TokenType::NATIVE)
        {
            return StatementType::FUNCTION;
        }
        else if (isTypeKeyword(next.type) || isModifierKeyword(next.type) || next.type == TokenType::IDENTIFIER)
        {
            // public/private followed by type keyword, modifier, or identifier (custom class) = variable declaration
            return StatementType::DECLARATION;
        }
        return StatementType::UNKNOWN;
    }

    StatementType StatementTypeDetector::analyzeFinalKeyword(const TokenStream& stream)
    {
        Token next = stream.peek();
        if (next.type == TokenType::CLASS)
        {
            return StatementType::CLASS;
        }
        else if (next.type == TokenType::INTERFACE)
        {
            return StatementType::INTERFACE;
        }
        return StatementType::UNKNOWN;
    }

    StatementType StatementTypeDetector::analyzeByKeywordCategory(TokenType currentType)
    {
        if (isControlFlowKeyword(currentType))
        {
            return StatementType::CONTROL_FLOW;
        }

        if (isLoopKeyword(currentType))
        {
            return StatementType::LOOP;
        }

        if (isFunctionKeyword(currentType))
        {
            return StatementType::FUNCTION;
        }

        if (isExceptionKeyword(currentType))
        {
            return StatementType::EXCEPTION;
        }

        if (isTypeKeyword(currentType) || isModifierKeyword(currentType))
        {
            return StatementType::DECLARATION;
        }

        return StatementType::UNKNOWN;
    }

    StatementType StatementTypeDetector::detectStatementType(const TokenStream& stream)
    {
        const Token& current = stream.current();

        // Handle simple token type cases
        switch (current.type)
        {
        case TokenType::CLASS:
            return StatementType::CLASS;
        case TokenType::INTERFACE:
            return StatementType::INTERFACE;
        case TokenType::LBRACE:
            return StatementType::BLOCK;
        case TokenType::SEMICOLON:
            return StatementType::EMPTY;
        case TokenType::IMPORT:
            return StatementType::IMPORT;
        case TokenType::END:
            return StatementType::UNKNOWN;
        case TokenType::PUBLIC:
        case TokenType::PRIVATE:
            {
                StatementType result = analyzeAccessModifier(stream);
                if (result != StatementType::UNKNOWN)
                    return result;
            }
            break;
        case TokenType::FINAL:
            {
                StatementType result = analyzeFinalKeyword(stream);
                if (result != StatementType::UNKNOWN)
                    return result;
            }
            break;
        case TokenType::IDENTIFIER:
            return analyzeIdentifierStatement(stream);
        default:
            break;
        }

        // Analyze by keyword category
        StatementType result = analyzeByKeywordCategory(current.type);
        if (result != StatementType::UNKNOWN)
        {
            return result;
        }

        // Default to expression statement
        return StatementType::EXPRESSION;
    }

    bool StatementTypeDetector::isControlFlowKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::IF:
        case TokenType::SWITCH:
        case TokenType::BREAK:
        case TokenType::CONTINUE:
        case TokenType::RETURN:
            return true;
        default:
            return false;
        }
    }

    bool StatementTypeDetector::isLoopKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::WHILE:
        case TokenType::DO:
        case TokenType::FOR:
            return true;
        default:
            return false;
        }
    }

    bool StatementTypeDetector::isTypeKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::INT:
        case TokenType::FLOAT:
        case TokenType::BOOL:
        case TokenType::STRING_TYPE:
        case TokenType::VOID:
            return true;
        default:
            return false;
        }
    }

    bool StatementTypeDetector::isModifierKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::FINAL:
        case TokenType::STATIC:
            return true;
        default:
            return false;
        }
    }

    bool StatementTypeDetector::isFunctionKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::FUNCTION:
        case TokenType::NATIVE:
            return true;
        default:
            return false;
        }
    }

    bool StatementTypeDetector::isExceptionKeyword(TokenType type) noexcept
    {
        switch (type)
        {
        case TokenType::TRY:
        case TokenType::THROW:
            return true;
        default:
            return false;
        }
    }

    StatementType StatementTypeDetector::analyzeIdentifierStatement(const TokenStream& stream)
    {
        Token nextToken = stream.peek();

        // Pattern: "ClassName varName" - this is a custom type declaration
        if (nextToken.type == TokenType::IDENTIFIER)
        {
            return StatementType::DECLARATION;
        }

        // Pattern: "Type[]" or "Type<...>" - array or generic type declaration
        if (nextToken.type == TokenType::LBRACKET || nextToken.type == TokenType::LESS)
        {
            std::string identifier = stream.current().stringValue.getString();

            // Check if this identifier is a type name
            if (identifier == "int" || identifier == "float" || identifier == "string" ||
                identifier == "bool" ||
                // Single-letter generic type parameters (T, K, V, etc.)
                (identifier.length() == 1 && std::isupper(identifier[0])) ||
                // Class names (start with uppercase letter)
                (!identifier.empty() && std::isupper(identifier[0])))
            {
                return StatementType::DECLARATION;
            }
            else
            {
                return StatementType::EXPRESSION;
            }
        }

        // Pattern: "identifier::..." - this is always an expression (qualified call/assignment/access)
        if (nextToken.type == TokenType::SCOPE)
        {
            return StatementType::EXPRESSION;
        }

        // Check for assignment operators
        if (nextToken.type == TokenType::ASSIGN ||
            nextToken.type == TokenType::PLUS_ASSIGN ||
            nextToken.type == TokenType::MINUS_ASSIGN ||
            nextToken.type == TokenType::MULTIPLY_ASSIGN ||
            nextToken.type == TokenType::DIVIDE_ASSIGN ||
            nextToken.type == TokenType::MODULO_ASSIGN)
        {
            return StatementType::ASSIGNMENT;
        }

        // Default to expression statement
        return StatementType::EXPRESSION;
    }

}
