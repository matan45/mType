#include "StatementTypeDetector.hpp"

namespace parser::utilities
{
    StatementType StatementTypeDetector::detectStatementType(const TokenStream& stream)
    {
        const Token& current = stream.current();

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
        default:
            break;
        }

        if (isControlFlowKeyword(current.type))
        {
            return StatementType::CONTROL_FLOW;
        }

        if (isLoopKeyword(current.type))
        {
            return StatementType::LOOP;
        }

        if (isFunctionKeyword(current.type))
        {
            return StatementType::FUNCTION;
        }

        if (isTypeKeyword(current.type) || isModifierKeyword(current.type))
        {
            return StatementType::DECLARATION;
        }

        if (current.type == TokenType::IDENTIFIER)
        {
            return analyzeIdentifierStatement(stream);
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

    bool StatementTypeDetector::looksLikeDeclaration(const TokenStream& stream)
    {
        // This method can be expanded for more sophisticated declaration detection
        return analyzeIdentifierStatement(stream) == StatementType::DECLARATION;
    }

    bool StatementTypeDetector::looksLikeAssignment(const TokenStream& stream)
    {
        return analyzeIdentifierStatement(stream) == StatementType::ASSIGNMENT;
    }
}