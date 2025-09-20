#pragma once
#include "../value/ValueType.hpp"
#include "../token/TokenType.hpp"
#include "TokenStream.hpp"
#include "../ast/GenericType.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <optional>

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
        
        // Recursive type information for nested collections
        std::optional<std::shared_ptr<TypeInfo>> elementTypeInfo;  // For Array, List, Set, Queue, Stack
        std::optional<std::shared_ptr<TypeInfo>> keyTypeInfo;      // For Map
        std::optional<std::shared_ptr<TypeInfo>> valueTypeInfo;    // For Map
        
        // Legacy fields for backward compatibility
        std::optional<ValueType> elementType;  // For Array, List, Set, Queue, Stack
        std::optional<ValueType> keyType;      // For Map
        std::optional<ValueType> valueType;    // For Map
        std::optional<std::string> elementClassName;  // For collections of custom objects
        std::optional<std::string> keyClassName;      // For Map with custom key objects  
        std::optional<std::string> valueClassName;    // For Map with custom value objects

        // Constructor for simple types
        TypeInfo(ValueType type) : baseType(type) {}
        
        // Constructor for object types
        TypeInfo(ValueType type, const std::string& className) : baseType(type), className(className) {}
        
        // Constructor for single-element collections (Array, List, Set, Queue, Stack) - legacy
        TypeInfo(ValueType baseType, ValueType elementType, const std::string& elementClassName = "")
            : baseType(baseType), elementType(elementType), elementClassName(elementClassName.empty() ? std::nullopt : std::make_optional(elementClassName)) {}
        
        // Constructor for Map - legacy
        TypeInfo(ValueType baseType, ValueType keyType, ValueType valueType, 
                const std::string& keyClassName = "", const std::string& valueClassName = "")
            : baseType(baseType), keyType(keyType), valueType(valueType),
              keyClassName(keyClassName.empty() ? std::nullopt : std::make_optional(keyClassName)),
              valueClassName(valueClassName.empty() ? std::nullopt : std::make_optional(valueClassName)) {}
              
        // NEW: Constructor for single-element collections with recursive TypeInfo
        TypeInfo(ValueType baseType, std::shared_ptr<TypeInfo> elementTypeInfo)
            : baseType(baseType), elementTypeInfo(elementTypeInfo) 
        {
            // Set legacy fields for backward compatibility
            if (elementTypeInfo) {
                elementType = elementTypeInfo->baseType;
                if (elementTypeInfo->baseType == ValueType::OBJECT) {
                    elementClassName = elementTypeInfo->className;
                }
            }
        }
        
        // NEW: Constructor for Map with recursive TypeInfo
        TypeInfo(ValueType baseType, std::shared_ptr<TypeInfo> keyTypeInfo, std::shared_ptr<TypeInfo> valueTypeInfo)
            : baseType(baseType), keyTypeInfo(keyTypeInfo), valueTypeInfo(valueTypeInfo)
        {
            // Set legacy fields for backward compatibility
            if (keyTypeInfo) {
                keyType = keyTypeInfo->baseType;
                if (keyTypeInfo->baseType == ValueType::OBJECT) {
                    keyClassName = keyTypeInfo->className;
                }
            }
            if (valueTypeInfo) {
                valueType = valueTypeInfo->baseType;
                if (valueTypeInfo->baseType == ValueType::OBJECT) {
                    valueClassName = valueTypeInfo->className;
                }
            }
        }
        
        // Helper methods for recursive type access
        bool hasNestedElementType() const { return elementTypeInfo.has_value(); }
        bool hasNestedKeyType() const { return keyTypeInfo.has_value(); }
        bool hasNestedValueType() const { return valueTypeInfo.has_value(); }
        
        std::shared_ptr<TypeInfo> getElementTypeInfo() const { return elementTypeInfo.value_or(nullptr); }
        std::shared_ptr<TypeInfo> getKeyTypeInfo() const { return keyTypeInfo.value_or(nullptr); }
        std::shared_ptr<TypeInfo> getValueTypeInfo() const { return valueTypeInfo.value_or(nullptr); }
        
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

    private:
        // Helper for qualified name parsing (Class::member)
        static std::string parseQualifiedName(TokenStream& stream);
    };
}