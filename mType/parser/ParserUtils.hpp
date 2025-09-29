#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <functional>
#include <memory>
#include "../value/ValueType.hpp"
#include "../value/ParameterType.hpp"
#include "../token/TokenType.hpp"
#include "../errors/SourceLocation.hpp"

// Forward declarations
namespace ast
{
    class ASTNode;
    class GenericType;
}

namespace parser
{
    /// @brief Centralized utility functions for parser operations
    /// Contains common functionality used across multiple parser classes
    class ParserUtils
    {
    public:
        /// @brief Efficiently build qualified names from parts (e.g., "Class::method")
        /// Optimized O(n) string concatenation instead of O(n²)
        /// @param parts Vector of name parts to join with "::"
        /// @return Qualified name string, empty if parts is empty
        [[nodiscard]] static std::string buildQualifiedName(const std::vector<std::string>& parts) noexcept;

        /// @brief Check if a string represents a valid identifier
        /// @param name String to validate
        /// @return true if valid identifier, false otherwise
        [[nodiscard]] static bool isValidIdentifier(std::string_view name) noexcept;

        /// @brief Validate function/method naming convention (must start with lowercase)
        /// @param name Function or method name to validate
        /// @param isStatic Whether this is a static method
        /// @param context Description for error message (e.g., "method", "function", "static method")
        /// @throws ParseException if name doesn't follow lowercase convention
        static void validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                     std::string_view context, const errors::SourceLocation& location);

        /// @brief Parse parameter list with centralized logic
        /// Eliminates code duplication across ClassParser and StatementParser methods
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-type pairs
        [[nodiscard]] static std::vector<std::pair<std::string, value::ValueType>>
        parseParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with generic type support (NEW)
        /// Parses function/method parameters with full generic type support
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-GenericType pairs
        [[nodiscard]] static std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>
        parseGenericParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with interface type support (NEW)
        /// Parses function/method parameters with interface/class type information preserved
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-ParameterType pairs
        [[nodiscard]] static std::vector<std::pair<std::string, value::ParameterType>>
        parseParameterListWithTypes(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse binary operators with left-associative precedence
        /// Eliminates code duplication across ExpressionParser binary operator methods
        /// @param stream Token stream to parse from  
        /// @param parseNext Function to parse next precedence level
        /// @param operators Vector of token types for this precedence level
        /// @return Binary expression tree
        [[nodiscard]] static std::unique_ptr<ast::ASTNode> parseBinaryOperators(
            class TokenStream& stream,
            std::function<std::unique_ptr<ast::ASTNode>()> parseNext,
            const std::vector<token::TokenType>& operators);

        /// @brief Convert compound assignment operator to binary operator
        /// Eliminates code duplication in assignment parsing
        /// @param compoundOp Compound assignment token type
        /// @return Corresponding binary operator token type
        /// @throws ParseException if not a valid compound assignment operator
        [[nodiscard]] static token::TokenType compoundToBinaryOperator(token::TokenType compoundOp);

        /// @brief Parse qualified identifier chain (e.g., namespace::Class::method)
        /// Eliminates code duplication in qualified name parsing
        /// @param stream Token stream to parse from
        /// @param initialName First part of qualified name
        /// @return Complete qualified name parts vector
        [[nodiscard]] static std::vector<std::string> parseQualifiedIdentifierChain(
            class TokenStream& stream,
            std::string_view initialName);

    private:
        // Utility class - no instances allowed
        ParserUtils() = delete;
        ~ParserUtils() = delete;
        ParserUtils(const ParserUtils&) = delete;
        ParserUtils& operator=(const ParserUtils&) = delete;
    };
}
