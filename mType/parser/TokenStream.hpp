#pragma once
#include "../token/Token.hpp"
#include "../token/TokenType.hpp"
#include "../lexer/Lexer.hpp"
#include <vector>

namespace parser
{
    using namespace token;
    using namespace lexer;
    using namespace errors;

    /// @brief Encapsulates token stream management for parsers
    /// Provides clean interface for token access without tight coupling
    class TokenStream
    {
    private:
        Lexer& lexer;
        Token currentToken;
        mutable std::vector<Token> lookaheadBuffer;
        bool hasRightShiftPending = false;  // Track if we have a pending > from splitting >>

    public:
        explicit TokenStream(Lexer& lex);
        ~TokenStream() = default;

        /// @brief Get current token without advancing
        [[nodiscard]] const Token& current() const noexcept { return currentToken; }

        /// @brief Get current token location
        [[nodiscard]] const SourceLocation& location() const noexcept { return currentToken.location; }

        /// @brief Advance to next token
        void advance();

        /// @brief Check if current token matches type without advancing
        [[nodiscard]] bool check(TokenType type) const noexcept;

        /// @brief Check if at end of input
        [[nodiscard]] bool isAtEnd() const noexcept;

        /// @brief Match current token type and advance if matches
        bool match(TokenType type);

        /// @brief Expect specific token type, advance if matches, throw if not
        void expect(TokenType type);

        /// @brief Expect a GREATER token, handling >> as two > tokens for generics
        /// If current token is RIGHT_SHIFT (>>), converts it to a single GREATER
        /// and leaves the second > for the next call
        void expectGreaterForGeneric();

        /// @brief Check if current token is GREATER or RIGHT_SHIFT (for generic closing)
        [[nodiscard]] bool checkGreaterForGeneric() const noexcept;

        /// @brief Peek at next token without advancing
        [[nodiscard]] Token peek() const;

        /// @brief Peek ahead multiple tokens without advancing
        [[nodiscard]] Token peekAhead(size_t offset) const;
    };
}
