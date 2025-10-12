#pragma once

#include "../../ast/GenericType.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations
namespace runtimeTypes::klass {
    class MethodDefinition;
    class ConstructorDefinition;
    class InterfaceRegistry;
}

namespace environment::registry {
    class ClassRegistry;
}

namespace ast::nodes::classes {
    class ClassNode;
}

namespace evaluator::utils
{
    using namespace ast;
    using namespace runtimeTypes::klass;

    /**
     * @brief Manages generic type instantiation, substitution, and validation
     *
     * This class handles the semantic analysis and runtime support for generics:
     * - Type parameter validation
     * - Generic class instantiation (Box<int> from Box<T>)
     * - Type substitution in methods and fields
     * - Generic constraint checking
     */
    class GenericTypeManager
    {
    public:
        /**
         * @brief Parse a generic instantiation string like "Box<int>" into base name and type arguments
         * @param instantiationName The full instantiation name (e.g., "Box<int>")
         * @return Pair of base name and vector of type argument strings
         */
        static std::pair<std::string, std::vector<std::string>> parseGenericInstantiation(
            const std::string& instantiationName);

        /**
         * @brief Create a specialized class definition from a generic class template
         * @param genericClass The generic class template (e.g., Box<T>)
         * @param typeArguments The concrete types to substitute (e.g., ["int"])
         * @return Specialized class definition (e.g., Box<int>)
         */
        static std::shared_ptr<ClassDefinition> instantiateGenericClass(
            std::shared_ptr<ClassDefinition> genericClass,
            const std::vector<std::string>& typeArguments);


        static auto referencesSubstitutionParameters(
            std::shared_ptr<ast::GenericType> genericType,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap) -> bool;

        /**
         * @brief Validate that type arguments match the generic parameters
         * @param genericClass The generic class definition
         * @param typeArguments The provided type arguments
         * @return True if valid, false otherwise
         */
        static bool validateTypeArguments(
            std::shared_ptr<ClassDefinition> genericClass,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Check if a string represents a generic instantiation (contains '<' and '>')
         * @param typeName The type name to check
         * @return True if it looks like a generic instantiation
         */
        static bool isGenericInstantiation(const std::string& typeName);

        /**
         * @brief Get the base class name from a generic instantiation
         * @param instantiationName Full name like "Box<int>"
         * @return Base name like "Box"
         */
        static std::string getBaseClassName(const std::string& instantiationName);

        /**
         * @brief Create a type substitution map for generic type resolution
         * @param genericParameters The generic type parameters (T, K, V, etc.)
         * @param typeArguments The concrete type arguments (int, string, etc.)
         * @return Map for type substitution
         */
        static std::unordered_map<std::string, std::string> createTypeSubstitutionMap(
            const std::vector<GenericTypeParameter>& genericParameters,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Substitute generic type parameters using AST-based approach
         * @param genericType The generic type AST node to substitute
         * @param substitutionMap Map of type parameter names to concrete GenericType objects
         * @return New GenericType with substitutions applied
         */
        static std::shared_ptr<ast::GenericType> substituteTypeParameters(
            std::shared_ptr<ast::GenericType> genericType,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        /**
         * @brief Create a GenericType substitution map from string type arguments
         * @param genericParameters The generic type parameters (T, K, V, etc.)
         * @param typeArguments The concrete type argument strings (int, string, etc.)
         * @return Map for AST-based type substitution
         */
        static std::unordered_map<std::string, std::shared_ptr<ast::GenericType>> createASTSubstitutionMap(
            const std::vector<GenericTypeParameter>& genericParameters,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Parse a type argument string to a GenericType object (handles nested generics)
         * @param typeArgument The type argument string (e.g., "String", "List<String>", "Map<String, Int>")
         * @return GenericType object representing the parsed type
         */
        static std::shared_ptr<ast::GenericType> parseTypeArgumentToGenericType(const std::string& typeArgument);

        /**
         * @brief Create a specialized method definition from a generic static method
         * @param genericMethod The generic static method template
         * @param typeArguments The concrete types to substitute
         * @return Specialized method definition
         */
        static std::shared_ptr<runtimeTypes::klass::MethodDefinition> instantiateStaticGenericMethod(
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> genericMethod,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Create a signature key for caching instantiated static generic methods
         * @param className The class name containing the method
         * @param methodName The method name
         * @param typeArguments The type arguments used
         * @return Unique signature key for caching
         */
        static std::string createStaticMethodSignatureKey(
            const std::string& className,
            const std::string& methodName,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Validate that type arguments are compatible with static method constraints
         * @param genericMethod The generic static method definition
         * @param typeArguments The provided type arguments
         * @return True if valid, false otherwise
         */
        static bool validateStaticMethodTypeArguments(
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> genericMethod,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Clear the generic class instantiation cache
         * This is useful for test isolation and preventing cache contamination
         */
        static void clearGenericClassCache();

        /**
         * @brief Check if a name is likely a generic type parameter
         * @param name The type name to check (e.g., "T", "Key", "Element")
         * @return True if it matches generic parameter patterns
         */
        static bool isGenericTypeParameter(const std::string& name);

        /**
         * @brief Check if a class name contains unresolved generic type parameters
         * @param className The class name to check (e.g., "List<T>", "Map<K,V>")
         * @return True if it contains unresolved generic parameters
         */
        static bool hasUnresolvedGenericParams(const std::string& className);

        /**
         * @brief Validate that type arguments satisfy generic parameter constraints
         * @param genericParameters The generic parameters with constraints (e.g., <T extends Comparable<T>>)
         * @param typeArguments The concrete type arguments to validate
         * @param classRegistry Shared pointer to ClassRegistry for class lookup
         * @param interfaceRegistry Shared pointer to InterfaceRegistry for interface lookup
         * @return True if all type arguments satisfy their constraints, false otherwise
         */
        static bool validateGenericConstraints(
            const std::vector<GenericTypeParameter>& genericParameters,
            const std::vector<std::string>& typeArguments,
            std::shared_ptr<environment::registry::ClassRegistry> classRegistry,
            std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry);

        /**
         * @brief Check if a specific class implements a required interface constraint
         * @param className The class name to check
         * @param interfaceName The interface name that must be implemented
         * @param classRegistry Shared pointer to ClassRegistry for class lookup
         * @param interfaceRegistry Shared pointer to InterfaceRegistry for interface lookup
         * @return True if the class implements the interface, false otherwise
         */
        static bool classImplementsInterface(
            const std::string& className,
            const std::string& interfaceName,
            std::shared_ptr<environment::registry::ClassRegistry> classRegistry,
            std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry);

    private:
        /**
         * @brief Parse type arguments from a generic instantiation
         * @param typeArgsString The content between < and >
         * @return Vector of individual type argument strings
         */
        static std::vector<std::string> parseTypeArguments(const std::string& typeArgsString);

        /**
         * @brief Convert GenericType to ValueType with generic parameter substitution (AST-based)
         * @param genericType The generic type to convert
         * @param substitutionMap AST-based map of type parameter to concrete GenericType
         * @return Converted ValueType with substitution applied
         */
        static value::ValueType convertGenericTypeToValueType(
            std::shared_ptr<ast::GenericType> genericType,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        /**
         * @brief Substitute generic type parameters in method types (AST-based)
         * @param originalMethod Original method definition
         * @param substitutionMap AST-based map of type parameter to concrete GenericType
         * @return New method definition with substituted types
         */
        static std::shared_ptr<runtimeTypes::klass::MethodDefinition> substituteMethodTypes(
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> originalMethod,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        /**
         * @brief Substitute generic type parameters in constructor types (AST-based)
         * @param originalConstructor Original constructor definition
         * @param substitutionMap AST-based map of type parameter to concrete GenericType
         * @return New constructor definition with substituted types
         */
        static std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> substituteConstructorTypes(
            std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> originalConstructor,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        // Helper methods for instantiateGenericClass
        static std::string createInstantiatedClassName(
            std::shared_ptr<ClassDefinition> genericClass,
            const std::vector<std::string>& typeArguments);

        static void copyAndSubstituteFields(
            std::shared_ptr<ClassDefinition> source,
            std::shared_ptr<ClassDefinition> target,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        static void copyAndSubstituteMethods(
            std::shared_ptr<ClassDefinition> source,
            std::shared_ptr<ClassDefinition> target,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

        static void copyAndSubstituteConstructors(
            std::shared_ptr<ClassDefinition> source,
            std::shared_ptr<ClassDefinition> target,
            const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap);

    private:
        /**
         * @brief Create a comprehensive cache key that prevents collisions
         * @param genericClass The generic class definition
         * @param typeArguments The concrete type arguments
         * @return Collision-resistant cache key with namespace and parameter info
         */
        static std::string createComprehensiveCacheKey(
            std::shared_ptr<ClassDefinition> genericClass,
            const std::vector<std::string>& typeArguments);

        /**
         * @brief Get reference to the global generic class cache and mutex
         * @return Tuple of mutex reference and cache reference
         */
        static std::pair<std::mutex&, std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>&>
            getGenericClassCache();

        /**
         * @brief Fast cache for common generic instantiation patterns
         * Uses numeric keys for O(1) lookup instead of string concatenation
         */
        struct FastCacheKey {
            std::string className;
            std::vector<std::string> typeArgs;

            // Pre-computed hash for O(1) comparison
            size_t hashValue;

            FastCacheKey(const std::string& name, const std::vector<std::string>& args);

            bool operator==(const FastCacheKey& other) const {
                return hashValue == other.hashValue &&
                       className == other.className &&
                       typeArgs == other.typeArgs;
            }
        };

        struct FastCacheKeyHasher {
            size_t operator()(const FastCacheKey& key) const {
                return key.hashValue;
            }
        };

        /**
         * @brief Get reference to fast cache for common patterns
         */
        static std::pair<std::mutex&, std::unordered_map<FastCacheKey, std::shared_ptr<ClassDefinition>, FastCacheKeyHasher>&>
            getFastGenericCache();

        /**
         * @brief Check if a generic instantiation is a common pattern eligible for fast cache
         * @param className The base class name
         * @param typeArguments The type arguments
         * @return true if this is a common pattern (List<T>, Map<K,V>, etc.)
         */
        static bool isCommonPattern(const std::string& className, const std::vector<std::string>& typeArguments);
    };
}