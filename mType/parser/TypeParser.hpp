#pragma once
#include "../value/ValueType.hpp"
#include "../token/TokenType.hpp"
#include "TokenStream.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>

namespace parser
{
    using namespace value;
    using namespace token;

    /// @brief Centralized type parsing utility with optimized lookups
    /// Eliminates code duplication across parsers
    class TypeParser
    {
    private:
        // Static lookup tables for O(1) type resolution
        static const std::unordered_map<TokenType, ValueType> tokenTypeMap;
        static const std::unordered_map<std::string_view, ValueType> stringTypeMap;
        static const std::unordered_set<TokenType> assignmentOperators;

    public:
        /// @brief Parse ValueType from token stream
        [[nodiscard]] static ValueType parseType(TokenStream& stream);
        
        /// @brief Parse type with class name for object types
        [[nodiscard]] static std::pair<ValueType, std::string> parseTypeWithClassName(TokenStream& stream);
        
        /// @brief Check if token is assignment operator
        [[nodiscard]] static bool isAssignmentOperator(TokenType tokenType) noexcept;
        
        /// @brief Convert TokenType to ValueType (for built-in types only)
        [[nodiscard]] static ValueType tokenToValueType(TokenType tokenType) noexcept;
        
        /// @brief Convert string to ValueType (for identifier-based types)
        [[nodiscard]] static ValueType stringToValueType(std::string_view typeName) noexcept;

    private:
        // Helper for qualified name parsing (Class::member)
        static std::string parseQualifiedName(TokenStream& stream);
    };
}