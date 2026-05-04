#pragma once
#include <string_view>
#include <cstdint>
#include "../token/Token.hpp"
#include "../token/TokenType.hpp"
#include "../errors/SourceLocation.hpp"

namespace lexer
{
    using namespace token;

    /**
     * Factory for creating tokens with proper values and locations
     * Responsibility: Centralized token creation with type-specific handling
     *
     * All string-carrying factories accept std::string_view and store it
     * directly in Token::stringValue. Lifetime invariant: the view must
     * remain valid for the owning Lexer's lifetime (see Token.hpp).
     */
    class TokenFactory
    {
    public:
        static Token createIntegerToken(int64_t value, const errors::SourceLocation& location);
        static Token createFloatToken(double value, const errors::SourceLocation& location);
        static Token createStringToken(std::string_view value, const errors::SourceLocation& location);
        static Token createIdentifierToken(std::string_view identifier, const errors::SourceLocation& location);
        static Token createKeywordToken(TokenType keywordType, std::string_view keyword,
                                        const errors::SourceLocation& location);

        static Token createOperatorToken(TokenType operatorType, std::string_view symbol,
                                         const errors::SourceLocation& location);

        static Token createEndToken(const errors::SourceLocation& location);
    };
}
