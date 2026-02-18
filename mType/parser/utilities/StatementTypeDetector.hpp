#pragma once
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"

namespace parser::utilities
{
    using namespace token;

    enum class StatementType
    {
        CONTROL_FLOW,
        LOOP,
        DECLARATION,
        ASSIGNMENT,
        FUNCTION,
        IMPORT,
        CLASS,
        INTERFACE,
        EXCEPTION,
        BLOCK,
        EXPRESSION,
        EMPTY,
        UNKNOWN
    };

    class StatementTypeDetector
    {
    public:
        static StatementType detectStatementType(const TokenStream& stream);

    private:
        static bool isControlFlowKeyword(TokenType type) noexcept;
        static bool isLoopKeyword(TokenType type) noexcept;
        static bool isTypeKeyword(TokenType type) noexcept;
        static bool isModifierKeyword(TokenType type) noexcept;
        static bool isFunctionKeyword(TokenType type) noexcept;
        static bool isExceptionKeyword(TokenType type) noexcept;

        static StatementType analyzeIdentifierStatement(const TokenStream& stream);

        // Check if a token is the contextual 'value' keyword (IDENTIFIER with string "value")
        static bool isValueKeyword(const Token& token) noexcept;

        // Helper methods for detectStatementType refactoring
        static StatementType analyzeAccessModifier(const TokenStream& stream);
        static StatementType analyzeFinalKeyword(const TokenStream& stream);
        static StatementType analyzeAbstractKeyword(const TokenStream& stream);
        static StatementType analyzeAnnotation(const TokenStream& stream);
        static StatementType analyzeByKeywordCategory(TokenType currentType);
    };
}
