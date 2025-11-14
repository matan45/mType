#pragma once

#include "../value/ValueType.hpp"
#include "../ast/GenericType.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/TypeResolutionException.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>

namespace types {

    /**
     * Helper class to traverse inheritance chains with cycle detection
     */
    class InheritanceChainTraverser {
    private:
        const std::unordered_map<std::string, std::string>& classInheritance;
        static const int MAX_DEPTH = 20;

    public:
        explicit InheritanceChainTraverser(const std::unordered_map<std::string, std::string>& inheritance);

        /**
         * Traverse inheritance chain to check if child extends target parent
         * @return true if childType has targetParent in its inheritance chain
         */
        bool traverse(const std::string& childType, const std::string& targetParent) const;

        /**
         * Calculate the inheritance distance from child to target parent
         * @return Number of inheritance steps, or -1 if no relationship exists
         */
        int calculateDistance(const std::string& childType, const std::string& targetParent) const;
    };

    /**
     * Helper class to parse generic type instantiations (e.g., "List<String>")
     */
    class GenericInstantiationParser {
    public:
        /**
         * Parse generic type instantiation into base name and type arguments
         * @param typeName The generic type string (e.g., "List<String>")
         * @return Pair of base name and vector of type arguments
         */
        static std::pair<std::string, std::vector<std::string>> parse(const std::string& typeName);
    };

    /**
     * Helper class to handle array type name creation and parsing
     */
    class ArrayTypeParser {
    public:
        /**
         * Create array type name from element type and dimensions
         * @param elementType The base element type
         * @param dimensions Number of array dimensions
         * @return Array type name (e.g., "String[][]")
         */
        static std::string createArrayTypeName(const std::string& elementType, int dimensions);

        /**
         * Parse array type name to extract element type and dimensions
         * @param arrayTypeName The array type name to parse
         * @return Pair of element type and dimension count
         */
        static std::pair<std::string, int> parseArrayTypeName(const std::string& arrayTypeName);
    };

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

        ExtendedTypeInfo();
        ExtendedTypeInfo(value::ValueType type, const std::string& name = "");
        ExtendedTypeInfo(value::ValueType type, const std::string& name, int dimensions);
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

        // Inheritance relationships
        std::unordered_map<std::string, std::string> classInheritance; // child -> parent
        mutable std::unordered_map<std::string, bool> subtypeCache; // For caching subtype relationships

        // Primitive to Box class mapping (lowercase to uppercase)
        // Maps: int -> Int, float -> Float, bool -> Bool, string -> String
        std::unordered_map<std::string, std::string> primitiveToBoxMapping;

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

        /**
         * Check if a type is a primitive type
         */
        bool isPrimitiveType(const std::string& typeName) const;

        /**
         * Get the Box class name for a primitive type name
         * @param primitiveName The primitive type name (e.g., "int", "float")
         * @return The Box class name (e.g., "Int", "Float"), or empty string if not a primitive
         */
        std::string getBoxClassName(const std::string& primitiveName) const;

        /**
         * Check if a type name needs to be mapped to a Box class
         * @param typeName The type name to check
         * @return true if this is a lowercase primitive name that should map to a Box class
         */
        bool shouldMapToBoxClass(const std::string& typeName) const;

        /**
         * NEW: Register inheritance relationship between classes
         */
        void registerInheritance(const std::string& childType, const std::string& parentType);

        /**
         * NEW: Check if one type is a subtype of another (inheritance-aware)
         * @return true if childType is a subtype of parentType
         */
        bool isSubtypeOf(const std::string& childType, const std::string& parentType) const;

        /**
         * NEW: Calculate inheritance distance between two types
         * @return Number of inheritance steps from childType to parentType, or -1 if no relationship
         */
        int getInheritanceDistance(const std::string& childType, const std::string& parentType) const;

    private:
        /**
         * Initialize built-in primitive types
         */
        void initializePrimitiveTypes();

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