#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "../../token/TokenType.hpp"

namespace ast
{
    class ASTNode;
}

namespace parser
{
    class TokenStream;

    /// @brief Shared parser utilities — operations that don't fit neatly into a
    /// single grammar region and are used by multiple top-level parsers.
    /// Specialized helpers live in their own headers: NameValidator,
    /// QualifiedNameParser, ParameterParser, AccessModifierParser.
    class ParserUtils
    {
    public:
        /// @brief Parse binary operators with left-associative precedence.
        [[nodiscard]] static std::unique_ptr<ast::ASTNode> parseBinaryOperators(
            TokenStream& stream,
            std::function<std::unique_ptr<ast::ASTNode>()> parseNext,
            const std::vector<token::TokenType>& operators);

        /// @brief Convert compound assignment operator to its binary operator.
        [[nodiscard]] static token::TokenType compoundToBinaryOperator(token::TokenType compoundOp);

        /// @brief Parse a comma-separated list of interfaces, optionally with
        /// generic parameters. Used by both extends and implements clauses.
        [[nodiscard]] static std::vector<std::string> parseInterfaceList(
            TokenStream& stream,
            std::string_view keywordName);

        /// @brief Parse a nested generic expression with depth tracking,
        /// preserving the textual representation (e.g. "Map<String, List<Int>>").
        [[nodiscard]] static std::string parseNestedGenericExpression(TokenStream& stream);

    private:
        /// @brief Throw a uniform "primitive in generic argument" ParseException.
        /// Shared between parseNestedGenericExpression and TypeParser so error
        /// messages stay identical across `implements`/`extends`/constraint
        /// paths and regular type-position parses.
        [[noreturn]] static void throwPrimitiveInGeneric(
            TokenStream& stream,
            std::string_view primitiveName,
            std::string_view boxedName);

        ParserUtils() = delete;
        ~ParserUtils() = delete;
        ParserUtils(const ParserUtils&) = delete;
        ParserUtils& operator=(const ParserUtils&) = delete;
    };
}
