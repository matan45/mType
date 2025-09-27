#pragma once

#include "../value/ValueType.hpp"
#include "../ast/GenericType.hpp"
#include "../exceptions/DomainExceptions.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>

namespace types {

    /**
     * Enhanced type information supporting arrays, generics, and custom types
     */
    struct ExtendedTypeInfo {
        value::ValueType baseType;
        std::string typeName;
        bool isArray;
        int arrayDimensions;
        bool isGeneric;
        std::vector<std::string> genericParameters;
        std::string fullyQualifiedName;

        ExtendedTypeInfo()
            : baseType(value::ValueType::VOID), isArray(false), arrayDimensions(0), isGeneric(false) {}

        ExtendedTypeInfo(value::ValueType type, const std::string& name = "")
            : baseType(type), typeName(name), isArray(false), arrayDimensions(0), isGeneric(false) {}

        ExtendedTypeInfo(value::ValueType type, const std::string& name, int dimensions)
            : baseType(type), typeName(name), isArray(dimensions > 0), arrayDimensions(dimensions), isGeneric(false) {}
    };

    /**
     * Exception thrown when type resolution fails
     * @deprecated Use mtype::exceptions::TypeResolutionException instead
     */
    class TypeResolutionException : public mtype::exceptions::TypeResolutionException {
    public:
        explicit TypeResolutionException(const std::string& message)
            : mtype::exceptions::TypeResolutionException(message, "", __FUNCTION__) {}
    };

    /**
     * Extensible type registry that replaces hardcoded type mappings
     * Supports custom types, arrays, generics, and user-defined collections
     */
    class TypeRegistry {
    private:
        // Core type mappings
        std::unordered_map<std::string, ExtendedTypeInfo> typeMap;
        std::unordered_map<std::string, value::ValueType> primitiveTypes;

        // Array type support
        std::unordered_map<std::string, ExtendedTypeInfo> arrayTypeCache;

        // Generic type support
        std::unordered_map<std::string, std::vector<std::string>> genericTypeParameters;
        std::unordered_set<std::string> knownGenericTypes;

        // Collection types
        std::unordered_set<std::string> collectionTypes;

        // Custom type factories
        std::unordered_map<std::string, std::function<ExtendedTypeInfo(const std::vector<std::string>&)>> typeFactories;

    public:
        /**
         * Initialize registry with built-in types
         */
        TypeRegistry();

        /**
         * Register a primitive type
         */
        void registerPrimitiveType(const std::string& name, value::ValueType valueType);

        /**
         * Register a custom class type
         */
        void registerCustomType(const std::string& name, const std::string& fullyQualifiedName = "");

        /**
         * Register a generic type with type parameters
         */
        void registerGenericType(const std::string& name, const std::vector<std::string>& typeParameters);

        /**
         * Register a collection type (List, Set, Map, etc.)
         */
        void registerCollectionType(const std::string& name, const std::vector<std::string>& typeParameters);

        /**
         * Register an array type for a given element type and dimensions
         */
        void registerArrayType(const std::string& elementType, int dimensions);

        /**
         * Check if a type is registered
         */
        bool hasType(const std::string& typeName) const;

        /**
         * Check if a type is a known array type
         */
        bool isArrayType(const std::string& typeName) const;

        /**
         * Check if a type is a known generic type
         */
        bool isGenericType(const std::string& typeName) const;

        /**
         * Check if a type is a collection type
         */
        bool isCollectionType(const std::string& typeName) const;

        /**
         * Resolve a type name to ExtendedTypeInfo
         */
        ExtendedTypeInfo resolveType(const std::string& typeName) const;

        /**
         * Resolve a type name to ValueType (for backward compatibility)
         */
        value::ValueType getValueType(const std::string& typeName) const;

        /**
         * Parse and resolve a complex type string (e.g., "Array<List<String>>")
         */
        ExtendedTypeInfo parseComplexType(const std::string& typeString) const;

        /**
         * Get type parameters for a generic type
         */
        std::vector<std::string> getGenericTypeParameters(const std::string& typeName) const;

        /**
         * Create array type name from element type and dimensions
         */
        static std::string createArrayTypeName(const std::string& elementType, int dimensions);

        /**
         * Parse array type name to get element type and dimensions
         */
        static std::pair<std::string, int> parseArrayTypeName(const std::string& arrayTypeName);

        /**
         * Check if a string represents a generic type parameter (T, K, V, etc.)
         */
        static bool isGenericParameter(const std::string& typeName);

        /**
         * Get all registered type names
         */
        std::vector<std::string> getAllTypeNames() const;

        /**
         * Clear all registered types (for testing)
         */
        void clear();

    private:
        /**
         * Initialize built-in primitive types
         */
        void initializePrimitiveTypes();

        /**
         * Initialize built-in collection types
         */
        void initializeCollectionTypes();

        /**
         * Parse generic type instantiation (e.g., "List<String>" -> {"List", ["String"]})
         */
        std::pair<std::string, std::vector<std::string>> parseGenericInstantiation(const std::string& typeName) const;

        /**
         * Validate type arguments for a generic type
         */
        bool validateTypeArguments(const std::string& genericType, const std::vector<std::string>& typeArgs) const;
    };

    /**
     * Global type registry instance
     */
    TypeRegistry& getGlobalTypeRegistry();

} // namespace types