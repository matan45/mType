#pragma once
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"

// Forward declarations
namespace ast
{
    class GenericType;
}

namespace parser
{
    /// @brief Specialized utility for parsing function/method parameter lists
    /// Handles parameter parsing with different type support (ValueType, GenericType, ParameterType)
    /// Follows Single Responsibility Principle - only handles parameter parsing
    class ParameterParser
    {
    public:
        /// @brief Parse parameter list with basic ValueType support
        /// Eliminates code duplication across ClassParser and StatementParser methods
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-type pairs
        [[nodiscard]] static std::vector<std::pair<std::string, value::ValueType>>
        parseParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with generic type support
        /// Parses function/method parameters with full generic type support
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-GenericType pairs
        [[nodiscard]] static std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>
        parseGenericParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with interface type support
        /// Parses function/method parameters with interface/class type information preserved
        /// @param stream Token stream to parse from
        /// @param expectParentheses Whether to expect and consume parentheses
        /// @return Vector of parameter name-ParameterType pairs
        [[nodiscard]] static std::vector<std::pair<std::string, value::ParameterType>>
        parseParameterListWithTypes(class TokenStream& stream, bool expectParentheses = true);

    private:
        // Utility class - no instances allowed
        ParameterParser() = delete;
        ~ParameterParser() = delete;
        ParameterParser(const ParameterParser&) = delete;
        ParameterParser& operator=(const ParameterParser&) = delete;
    };
}
