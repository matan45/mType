#include "StatementTypeDetector.hpp"

namespace parser::utilities
{
    bool StatementTypeDetector::isValueKeyword(const Token& token) noexcept
    {
        return token.type == TokenType::IDENTIFIER &&
               token.stringValue == "value";
    }

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
        else if (next.type == TokenType::FUNCTION)
        {
            return StatementType::FUNCTION;
        }
        // Check if next is 'value' contextual keyword: public value class ...
        else if (isValueKeyword(next))
        {
            Token afterValue = stream.peekAhead(2);
            if (afterValue.type == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
            // 'value' used as identifier (e.g., public int value), fall through to DECLARATION
            return StatementType::DECLARATION;
        }
        // PHASE 4 FIX: Check if next is another modifier (final/abstract) that might precede class/interface
        else if (next.type == TokenType::FINAL || next.type == TokenType::ABSTRACT)
        {
            // Look ahead one more token: public final CLASS or public abstract CLASS
            Token afterModifier = stream.peekAhead(2);
            if (afterModifier.type == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
            else if (afterModifier.type == TokenType::INTERFACE)
            {
                return StatementType::INTERFACE;
            }
            else if (afterModifier.type == TokenType::FUNCTION)
            {
                return StatementType::FUNCTION;
            }
            // Check for: public final value class ... or public abstract value class ...
            if (isValueKeyword(afterModifier))
            {
                Token afterValue = stream.peekAhead(3);
                if (afterValue.type == TokenType::CLASS)
                {
                    return StatementType::CLASS;
                }
            }
            // Otherwise, it's a declaration: public final int x;
            return StatementType::DECLARATION;
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
        // final value class ...
        else if (isValueKeyword(next))
        {
            Token afterValue = stream.peekAhead(2);
            if (afterValue.type == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
        }
        return StatementType::UNKNOWN;
    }

    StatementType StatementTypeDetector::analyzeAbstractKeyword(const TokenStream& stream)
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
        else if (next.type == TokenType::FUNCTION)
        {
            return StatementType::FUNCTION;
        }
        // abstract value class ... (will be rejected by parser, but route to CLASS for proper error)
        else if (isValueKeyword(next))
        {
            Token afterValue = stream.peekAhead(2);
            if (afterValue.type == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
        }
        return StatementType::UNKNOWN;
    }

    StatementType StatementTypeDetector::analyzeAnnotation(const TokenStream& stream)
    {
        // Skip past all consecutive annotations (@Override @Script etc)
        // to find what they're annotating
        // Use peek to avoid consuming tokens from the lexer
        size_t lookAheadIndex = 0;

        // Helper lambda to get token at index (0 = current, 1+ = peekAhead)
        // peekAhead(N) returns the token N positions ahead from current
        auto getTokenAt = [&stream](size_t index) -> Token {
            if (index == 0) return stream.current();
            return stream.peekAhead(index);
        };

        while (getTokenAt(lookAheadIndex).type == TokenType::AT)
        {
            lookAheadIndex++; // Skip @
            if (getTokenAt(lookAheadIndex).type == TokenType::IDENTIFIER)
            {
                lookAheadIndex++; // Skip annotation name
            }
            else
            {
                return StatementType::UNKNOWN;
            }

            // Skip annotation parameters if present (e.g., @Throw(IOException, NetworkException))
            if (getTokenAt(lookAheadIndex).type == TokenType::LPAREN)
            {
                lookAheadIndex++; // Skip LPAREN

                // Skip all tokens until we find the matching RPAREN
                int parenDepth = 1;
                while (parenDepth > 0 && getTokenAt(lookAheadIndex).type != TokenType::END)
                {
                    TokenType current = getTokenAt(lookAheadIndex).type;
                    if (current == TokenType::LPAREN)
                    {
                        parenDepth++;
                    }
                    else if (current == TokenType::RPAREN)
                    {
                        parenDepth--;
                    }
                    lookAheadIndex++;
                }

                // If we didn't find matching RPAREN, annotation is malformed
                if (parenDepth != 0)
                {
                    return StatementType::UNKNOWN;
                }
            }
        }

        // Now check what comes after the annotation(s)
        Token afterAnnotationsToken = getTokenAt(lookAheadIndex);
        TokenType afterAnnotations = afterAnnotationsToken.type;

        // Annotations can precede: classes, interfaces, annotation declarations,
        // functions, or methods.
        if (afterAnnotations == TokenType::CLASS)
        {
            return StatementType::CLASS;
        }
        else if (afterAnnotations == TokenType::INTERFACE)
        {
            return StatementType::INTERFACE;
        }
        else if (afterAnnotations == TokenType::ANNOTATION)
        {
            return StatementType::ANNOTATION_DECLARATION;
        }
        else if (afterAnnotations == TokenType::FUNCTION)
        {
            return StatementType::FUNCTION;
        }
        // Annotations with access modifiers: @Override public function ...
        else if (afterAnnotations == TokenType::PUBLIC || afterAnnotations == TokenType::PRIVATE)
        {
            lookAheadIndex++; // Skip access modifier
            TokenType afterModifier = getTokenAt(lookAheadIndex).type;
            if (afterModifier == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
            else if (afterModifier == TokenType::INTERFACE)
            {
                return StatementType::INTERFACE;
            }
            else if (afterModifier == TokenType::FUNCTION)
            {
                return StatementType::FUNCTION;
            }
            // If followed by type/identifier, it's likely a declaration with annotation
            else if (isTypeKeyword(afterModifier) || afterModifier == TokenType::IDENTIFIER)
            {
                return StatementType::DECLARATION;
            }
        }
        // Annotations with value contextual keyword: @Annotation value class ...
        else if (isValueKeyword(afterAnnotationsToken))
        {
            lookAheadIndex++; // Skip 'value'
            TokenType afterValue = getTokenAt(lookAheadIndex).type;
            if (afterValue == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
        }
        // Annotations with final/abstract: @Override abstract function ...
        else if (afterAnnotations == TokenType::FINAL || afterAnnotations == TokenType::ABSTRACT)
        {
            lookAheadIndex++; // Skip modifier
            Token afterModifierToken = getTokenAt(lookAheadIndex);
            TokenType afterModifier = afterModifierToken.type;
            if (afterModifier == TokenType::CLASS)
            {
                return StatementType::CLASS;
            }
            else if (afterModifier == TokenType::INTERFACE)
            {
                return StatementType::INTERFACE;
            }
            else if (afterModifier == TokenType::FUNCTION)
            {
                return StatementType::FUNCTION;
            }
            // final/abstract followed by value class
            if (isValueKeyword(afterModifierToken))
            {
                lookAheadIndex++;
                if (getTokenAt(lookAheadIndex).type == TokenType::CLASS)
                {
                    return StatementType::CLASS;
                }
            }
        }
        // Annotations directly on typed declarations: @Deprecated int x;
        else if (isTypeKeyword(afterAnnotations) || afterAnnotations == TokenType::IDENTIFIER)
        {
            return StatementType::DECLARATION;
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
        case TokenType::ANNOTATION:
            return StatementType::ANNOTATION_DECLARATION;
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
        case TokenType::ABSTRACT:
            {
                StatementType result = analyzeAbstractKeyword(stream);
                if (result != StatementType::UNKNOWN)
                    return result;
            }
            break;
        case TokenType::AT:
            {
                StatementType result = analyzeAnnotation(stream);
                if (result != StatementType::UNKNOWN)
                    return result;
            }
            break;
        case TokenType::IDENTIFIER:
            {
                // Check for 'value' contextual keyword: value class ...
                if (isValueKeyword(current))
                {
                    Token next = stream.peek();
                    if (next.type == TokenType::CLASS)
                        return StatementType::CLASS;
                }
                return analyzeIdentifierStatement(stream);
            }
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
        case TokenType::MATCH:
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

        // Pattern: "ClassName? varName" - nullable type declaration
        if (nextToken.type == TokenType::QUESTION)
        {
            return StatementType::DECLARATION;
        }

        // Pattern: "Type[]" or "Type<...>" - array or generic type declaration
        if (nextToken.type == TokenType::LBRACKET || nextToken.type == TokenType::LESS)
        {
            std::string identifier = std::string(stream.current().stringValue);

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
