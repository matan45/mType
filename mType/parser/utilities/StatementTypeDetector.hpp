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

        static StatementType analyzeIdentifierStatement(const TokenStream& stream);
        static bool looksLikeDeclaration(const TokenStream& stream);
        static bool looksLikeAssignment(const TokenStream& stream);
    };
}