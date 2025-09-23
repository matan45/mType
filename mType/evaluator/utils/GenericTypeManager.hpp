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

        /**
         * @brief Create a specialized class definition from a generic ClassNode
         * @param genericClassNode The generic class AST node (e.g., Box<T>)
         * @param typeArguments The concrete types to substitute (e.g., ["int"])
         * @return Specialized class definition (e.g., Box<int>)
         */
        static std::shared_ptr<ClassDefinition> instantiateGenericClassFromNode(
            ast::nodes::classes::ClassNode* genericClassNode,
            const std::vector<std::string>& typeArguments);

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
         * @brief Substitute generic type parameters in a type string
         * @param typeString Original type string (may contain T, K, etc.)
         * @param substitutionMap Map of type parameter to concrete type
         * @return Type string with substitutions applied
         */
        static std::string substituteTypeParameters(
            const std::string& typeString,
            const std::unordered_map<std::string, std::string>& substitutionMap);

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

    private:
        /**
         * @brief Parse type arguments from a generic instantiation
         * @param typeArgsString The content between < and >
         * @return Vector of individual type argument strings
         */
        static std::vector<std::string> parseTypeArguments(const std::string& typeArgsString);

        /**
         * @brief Substitute generic type parameters in field types
         * @param originalType Original field type
         * @param substitutionMap Map of type parameter to concrete type
         * @return Substituted field type
         */
        static value::ValueType substituteFieldType(
            value::ValueType originalType,
            const std::unordered_map<std::string, std::string>& substitutionMap);

        /**
         * @brief Convert GenericType to ValueType with generic parameter substitution
         * @param genericType The generic type to convert
         * @param substitutionMap Map of type parameter to concrete type
         * @return Converted ValueType with substitution applied
         */
        static value::ValueType convertGenericTypeToValueType(
            std::shared_ptr<ast::GenericType> genericType,
            const std::unordered_map<std::string, std::string>& substitutionMap);

        /**
         * @brief Substitute generic type parameters in method types
         * @param originalMethod Original method definition
         * @param substitutionMap Map of type parameter to concrete type
         * @return New method definition with substituted types
         */
        static std::shared_ptr<runtimeTypes::klass::MethodDefinition> substituteMethodTypes(
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> originalMethod,
            const std::unordered_map<std::string, std::string>& substitutionMap);

        /**
         * @brief Substitute generic type parameters in constructor types
         * @param originalConstructor Original constructor definition
         * @param substitutionMap Map of type parameter to concrete type
         * @return New constructor definition with substituted types
         */
        static std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> substituteConstructorTypes(
            std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> originalConstructor,
            const std::unordered_map<std::string, std::string>& substitutionMap);
    };
}