#pragma once
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "ParameterDeclaration.hpp"

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
        /// @return Vector of parameter name-type pairs (annotations discarded)
        [[nodiscard]] static std::vector<std::pair<std::string, value::ValueType>>
        parseParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with generic type support
        /// @return Vector of parameter name-GenericType pairs (annotations discarded)
        [[nodiscard]] static std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>
        parseGenericParameterList(class TokenStream& stream, bool expectParentheses = true);

        /// @brief Parse parameter list with interface type support
        /// @return Vector of parameter name-ParameterType pairs (annotations discarded)
        [[nodiscard]] static std::vector<std::pair<std::string, value::ParameterType>>
        parseParameterListWithTypes(class TokenStream& stream, bool expectParentheses = true);

        // MYT-110: annotation-aware variants. Each parameter may be preceded
        // by a chain of `@X` annotations, collected via AnnotationParser.
        // Only the GenericType and ParameterType flavors are needed today —
        // the ValueType variant is callable via parseParameterList() which
        // tolerates but discards any `@X` chain.
        [[nodiscard]] static std::vector<ParameterDeclaration>
        parseGenericParameterDeclList(class TokenStream& stream, bool expectParentheses = true);

        [[nodiscard]] static std::vector<TypedParameterDeclaration>
        parseTypedParameterDeclList(class TokenStream& stream, bool expectParentheses = true);

    private:
        // Utility class - no instances allowed
        ParameterParser() = delete;
        ~ParameterParser() = delete;
        ParameterParser(const ParameterParser&) = delete;
        ParameterParser& operator=(const ParameterParser&) = delete;
    };
}
