#pragma once

#include "../../ast/VisibilityModifier.hpp"
#include "../../token/TokenType.hpp"
#include "../TokenStream.hpp"
#include "../../errors/SourceLocation.hpp"

namespace parser::utilities
{
    using namespace ast;
    using namespace token;
    using namespace errors;

    /**
     * @brief Utility class for parsing visibility modifiers (public/private) for top-level declarations
     *
     * Handles parsing of visibility modifiers for:
     * - Classes
     * - Interfaces
     * - Functions
     * - Global variables (future support)
     *
     * Default behavior: PUBLIC if no modifier is specified
     */
    class VisibilityParser
    {
    public:
        /**
         * @brief Parse visibility modifier from token stream
         * @param tokenStream The token stream to parse from
         * @param defaultModifier The default visibility if none is specified (defaults to PUBLIC)
         * @return The parsed visibility modifier
         *
         * Example usage:
         * ```
         * VisibilityModifier vis = VisibilityParser::parseVisibilityModifier(stream);
         * // If "public" or "private" is found, advances the token stream
         * // Otherwise, returns PUBLIC (default)
         * ```
         */
        static VisibilityModifier parseVisibilityModifier(
            TokenStream& tokenStream,
            VisibilityModifier defaultModifier = VisibilityModifier::PUBLIC);

        /**
         * @brief Check if a token type is a visibility modifier
         * @param type The token type to check
         * @return true if the token is PUBLIC or PRIVATE, false otherwise
         */
        static bool isVisibilityModifier(TokenType type);

    private:
        /**
         * @brief Convert TokenType to VisibilityModifier
         * @param type The token type (must be PUBLIC or PRIVATE)
         * @return The corresponding visibility modifier
         * @throws std::invalid_argument if token type is not a visibility modifier
         */
        static VisibilityModifier tokenTypeToVisibilityModifier(TokenType type);
    };
}
