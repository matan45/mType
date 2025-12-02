#pragma once
#include <string_view>
#include <string>
#include "../token/Token.hpp"
#include "../token/TokenType.hpp"
#include "../errors/SourceLocation.hpp"

namespace lexer
{
    using namespace token;

    /**
     * Factory for creating tokens with proper values and locations
     * Responsibility: Centralized token creation with type-specific handling
     */
    class TokenFactory
    {
    public:
        // Type-specific token creation
        static Token createIntegerToken(int64_t value, const errors::SourceLocation& location);
        static Token createFloatToken(float value, const errors::SourceLocation& location);
        static Token createStringToken(std::string_view value, const errors::SourceLocation& location);
        static Token createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location);
        static Token createKeywordToken(TokenType keywordType, std::string_view keyword,
                                        const errors::SourceLocation& location);

        // Operator tokens
        static Token createOperatorToken(TokenType operatorType, std::string_view symbol,
                                         const errors::SourceLocation& location);

        // Special tokens
        static Token createEndToken(const errors::SourceLocation& location);

    private:
        // Internal helpers for string pool access
        static value::InternedString internString(std::string_view str);
        static value::InternedString emptyString();
    };
}
