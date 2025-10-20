#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <functional>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"

// Forward declarations
namespace ast
{
    class ASTNode;
    class GenericType;
}

namespace parser
{
    /// @brief General-purpose parser utility functions
    /// Contains common functionality for parsing operations
    /// Note: Specialized utilities have been extracted into:
    /// - ParameterParser (parameter list parsing)
    /// - NameValidator (name validation)
    /// - QualifiedNameParser (qualified name operations)
    class ParserUtils
    {
    public:
        // ============================================================================
        // LEGACY FORWARDING METHODS - For backward compatibility
        // These forward to the appropriate specialized utility classes
        // New code should use the specialized classes directly
        // ============================================================================

        /// @brief Build qualified names from parts (forwards to QualifiedNameParser)
        /// @deprecated Use QualifiedNameParser::buildQualifiedName directly
        [[nodiscard]] static std::string buildQualifiedName(const std::vector<std::string>& parts) noexcept;

        /// @brief Check if a string represents a valid identifier (forwards to NameValidator)
        /// @deprecated Use NameValidator::isValidIdentifier directly
        [[nodiscard]] static bool isValidIdentifier(std::string_view name) noexcept;

        /// @brief Validate function/method naming convention (forwards to NameValidator)
        /// @deprecated Use NameValidator::validateFunctionNamingConvention directly
        static void validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                     std::string_view context, const errors::SourceLocation& location);

        /// @brief Parse parameter list (forwards to ParameterParser)
        /// @deprecated Use ParameterParser::parseParameterList directly
        [[nodiscard]] static std::vector<std::pair<std::string, value::ValueType>>
        parseParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with generic type support (forwards to ParameterParser)
        /// @deprecated Use ParameterParser::parseGenericParameterList directly
        [[nodiscard]] static std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>
        parseGenericParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with interface type support (forwards to ParameterParser)
        /// @deprecated Use ParameterParser::parseParameterListWithTypes directly
        [[nodiscard]] static std::vector<std::pair<std::string, value::ParameterType>>
        parseParameterListWithTypes(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse qualified identifier chain (forwards to QualifiedNameParser)
        /// @deprecated Use QualifiedNameParser::parseQualifiedIdentifierChain directly
        [[nodiscard]] static std::vector<std::string> parseQualifiedIdentifierChain(
            class TokenStream& stream,
            std::string_view initialName);

        /// @brief Validate that a name starts with capital letter (forwards to NameValidator)
        /// @deprecated Use NameValidator::validateCapitalizedName directly
        static void validateCapitalizedName(std::string_view name,
                                            std::string_view context,
                                            const errors::SourceLocation& location);

        /// @brief Validate that a name is a valid identifier (forwards to NameValidator)
        /// @deprecated Use NameValidator::validateIdentifierName directly
        static void validateIdentifierName(std::string_view name,
                                           std::string_view context,
                                           const errors::SourceLocation& location);

        // ============================================================================
        // CORE PARSER UTILITIES - Remain in ParserUtils
        // These are general-purpose parsing operations
        // ============================================================================

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

        /// @brief Parse comma-separated list of interfaces with optional generic parameters
        /// Eliminates code duplication between InterfaceParser and ClassDeclarationParser
        /// @param stream Token stream to parse from
        /// @param keywordName Name of keyword ("extends" or "implements") for error messages
        /// @return Vector of interface names with generic parameters
        [[nodiscard]] static std::vector<std::string> parseInterfaceList(
            class TokenStream& stream,
            std::string_view keywordName);

        /// @brief Parse nested generic expression with depth tracking (e.g., Map<String, List<Int>>)
        /// Eliminates code duplication in complex generic parsing
        /// @param stream Token stream to parse from
        /// @return String representation of nested generic expression
        [[nodiscard]] static std::string parseNestedGenericExpression(class TokenStream& stream);

    private:
        // Utility class - no instances allowed
        ParserUtils() = delete;
        ~ParserUtils() = delete;
        ParserUtils(const ParserUtils&) = delete;
        ParserUtils& operator=(const ParserUtils&) = delete;
    };
}
