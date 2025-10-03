#pragma once

#include "../../ast/AccessModifier.hpp"
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"

namespace parser::utilities
{
    using namespace ast;
    using namespace token;
    using namespace errors;

    /**
     * @brief Utility class for parsing access modifiers from token streams
     *
     * Provides centralized logic for detecting and parsing private, public,
     * and protected access modifiers in class and interface definitions.
     */
    class AccessModifierParser
    {
    public:
        /**
         * @brief Parse optional access modifier from current token position
         *
         * If current token is an access modifier (PRIVATE, PUBLIC, PROTECTED),
         * consumes the token and returns the corresponding AccessModifier.
         * Otherwise, returns the provided default modifier.
         *
         * @param tokenStream The token stream to parse from
         * @param defaultModifier The default to return if no modifier present
         * @return The parsed access modifier or default
         */
        static AccessModifier parseAccessModifier(
            TokenStream& tokenStream,
            AccessModifier defaultModifier);

        /**
         * @brief Check if current token is an access modifier keyword
         *
         * @param type The token type to check
         * @return true if token is PRIVATE, PUBLIC, or PROTECTED
         */
        static bool isAccessModifier(TokenType type);

        /**
         * @brief Validate modifier is appropriate for context (class vs interface)
         *
         * Interfaces only allow PUBLIC modifiers. This method throws a
         * ParseException if an invalid modifier is used in an interface.
         *
         * @param modifier The access modifier to validate
         * @param isInterface true if in interface context, false for class
         * @param location Source location for error reporting
         * @throws ParseException if private/protected used in interface
         */
        static void validateModifierForContext(
            AccessModifier modifier,
            bool isInterface,
            const SourceLocation& location);

        /**
         * @brief Convert TokenType to AccessModifier
         *
         * @param type Token type (must be PRIVATE, PUBLIC, or PROTECTED)
         * @return Corresponding AccessModifier
         * @throws std::invalid_argument if type is not an access modifier
         */
        static AccessModifier tokenTypeToAccessModifier(TokenType type);
    };
}
