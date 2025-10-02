#pragma once
#include "TokenStream.hpp"
#include "../value/ValueType.hpp"
#include "../token/TokenType.hpp"
#include "../ast/GenericType.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>

namespace parser
{
    using namespace value;
    using namespace token;

    // Forward declaration for recursive TypeInfo
    struct TypeInfo;
    
    /// @brief Holds complete type information including generic parameters with recursive support
    struct TypeInfo
    {
        ValueType baseType;
        std::string className;  // For custom classes
        
        // No collection-specific fields needed anymore

        // Default constructor
        TypeInfo() : baseType(ValueType::VOID) {}

        // Constructor for simple types
        TypeInfo(ValueType type) : baseType(type) {}

        // Constructor for object types
        TypeInfo(ValueType type, const std::string& className) : baseType(type), className(className) {}
        
        // No collection-specific constructors needed anymore
        
        // No collection-specific helper methods needed anymore
        
        // Convert TypeInfo to string representation (for backward compatibility)
        std::string toString() const;
    };

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
        /// @brief Parse ValueType from token stream (legacy method)
        [[nodiscard]] static ValueType parseType(TokenStream& stream);

        /// @brief Parse complete type information including generics
        [[nodiscard]] static TypeInfo parseTypeInfo(TokenStream& stream);

        /// @brief Parse type information and convert to GenericType (for new generic system)
        [[nodiscard]] static std::shared_ptr<ast::GenericType> parseGenericType(TokenStream& stream);

        /// @brief Parse type with class name for object types (legacy method)
        [[nodiscard]] static std::pair<ValueType, std::string> parseTypeWithClassName(TokenStream& stream);

        /// @brief Check if token is assignment operator
        [[nodiscard]] static bool isAssignmentOperator(TokenType tokenType) noexcept;

        /// @brief Convert TokenType to ValueType (for built-in types only)
        [[nodiscard]] static ValueType tokenToValueType(TokenType tokenType) noexcept;

        /// @brief Convert string to ValueType (for identifier-based types)
        [[nodiscard]] static ValueType stringToValueType(std::string_view typeName) noexcept;

        /// @brief Create TypeInfo from class name string (handles basic types and custom classes)
        [[nodiscard]] static TypeInfo createTypeInfoFromClassName(const std::string& className);

        /// @brief Convert TypeInfo to GenericType (for new generic system)
        [[nodiscard]] static std::shared_ptr<ast::GenericType> convertTypeInfoToGenericType(const TypeInfo& typeInfo);

    private:
        // Helper for qualified name parsing (Class::member)
        static std::string parseQualifiedName(TokenStream& stream);
    };
}
