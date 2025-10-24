#pragma once

#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace ast {
namespace nodes {
namespace classes {
    class MemberAccessNode;
    class MethodCallNode;
}
}
}

namespace evaluator
{
    namespace interfaces
    {
        using namespace value;
        using namespace runtimeTypes::klass;
        using ast::ASTNode;

        /**
         * @brief Interface for object-oriented operations
         *
         * Provides abstraction for object evaluation, enabling:
         * - Dependency Inversion: High-level modules depend on abstraction
         * - Testing: Mock implementations for unit testing
         * - Flexibility: Multiple object model implementations
         *
         * Design Principles:
         * - Interface Segregation: Focused on object operations
         * - Open/Closed: New implementations without modifying dependents
         * - Single Responsibility: Only object-related operations
         */
        class IObjectEvaluator
        {
        public:
            virtual ~IObjectEvaluator() = default;

            /**
             * @brief Evaluate an object-related AST node
             * @param node The node to evaluate
             * @return The evaluated value
             */
            virtual Value evaluate(ASTNode* node) = 0;

            /**
             * @brief Check if this evaluator can handle a given node
             * @param node The node to check
             * @return true if this evaluator can handle the node
             */
            virtual bool canHandle(ASTNode* node) const = 0;

            /**
             * @brief Call a method on an object instance
             * @param instance The object instance
             * @param methodName The name of the method to call
             * @param args The arguments to pass
             * @return The return value of the method
             */
            virtual Value callMethod(
                std::shared_ptr<ObjectInstance> instance,
                const std::string& methodName,
                const std::vector<Value>& args) = 0;

            /**
             * @brief Call a method on an object instance with source location
             * @param instance The object instance
             * @param methodName The name of the method to call
             * @param args The arguments to pass
             * @param location Source location for error reporting
             * @return The return value of the method
             */
            virtual Value callMethod(
                std::shared_ptr<ObjectInstance> instance,
                const std::string& methodName,
                const std::vector<Value>& args,
                const errors::SourceLocation& location) = 0;

            /**
             * @brief Create an instance of a class
             * @param className The name of the class
             * @param constructorArgs Arguments for the constructor
             * @param location Source location for error reporting
             * @return The created instance
             */
            virtual std::shared_ptr<ObjectInstance> createInstance(
                const std::string& className,
                const std::vector<Value>& constructorArgs,
                const errors::SourceLocation& location = errors::SourceLocation{}) = 0;

            /**
             * @brief Create an instance with generic type bindings
             * @param className The name of the class (possibly generic)
             * @param constructorArgs Arguments for the constructor
             * @param typeBindings Generic type parameter bindings
             * @param location Source location for error reporting
             * @return The created instance
             */
            virtual std::shared_ptr<ObjectInstance> createInstanceWithTypeBindings(
                const std::string& className,
                const std::vector<Value>& constructorArgs,
                const std::unordered_map<std::string, std::string>& typeBindings,
                const errors::SourceLocation& location = errors::SourceLocation{}) = 0;

            /**
             * @brief Access a static member of a class
             * @param className The name of the class
             * @param memberName The name of the static member
             * @return The value of the static member
             */
            virtual Value accessStaticMember(
                const std::string& className,
                const std::string& memberName) = 0;

            /**
             * @brief Assign a value to a static member of a class
             * @param className The name of the class
             * @param memberName The name of the static member
             * @param value The value to assign
             */
            virtual void assignStaticMember(
                const std::string& className,
                const std::string& memberName,
                const Value& value) = 0;

            /**
             * @brief Call a static method on a class
             * @param className The name of the class
             * @param methodName The name of the static method
             * @param args The arguments to pass
             * @param location Source location for error reporting
             * @return The return value of the method
             */
            virtual Value callStaticMethod(
                const std::string& className,
                const std::string& methodName,
                const std::vector<Value>& args,
                const errors::SourceLocation& location = errors::SourceLocation{}) = 0;

            /**
             * @brief Access an instance member
             * @param object The object instance
             * @param memberName The name of the member
             * @param location Source location for error reporting
             * @return The value of the member
             */
            virtual Value accessMember(
                std::shared_ptr<ObjectInstance> object,
                const std::string& memberName,
                const errors::SourceLocation& location = errors::SourceLocation{}) = 0;

            /**
             * @brief Assign a value to an instance member
             * @param object The object instance
             * @param memberName The name of the member
             * @param value The value to assign
             * @param location Source location for error reporting
             */
            virtual void assignMember(
                std::shared_ptr<ObjectInstance> object,
                const std::string& memberName,
                const Value& value,
                const errors::SourceLocation& location = errors::SourceLocation{}) = 0;

            /**
             * @brief Evaluate a member access node
             * @param node The member access node
             * @return The evaluated value
             */
            virtual Value evaluateMemberAccessNode(
                ast::nodes::classes::MemberAccessNode* node) = 0;

            /**
             * @brief Evaluate a method call node
             * @param node The method call node
             * @return The evaluated value
             */
            virtual Value evaluateMethodCallNode(
                ast::nodes::classes::MethodCallNode* node) = 0;

            /**
             * @brief Resolve a type parameter from the current context
             * @param typeParam The type parameter name to resolve
             * @return The resolved type name
             */
            virtual std::string resolveTypeParameterFromContext(
                const std::string& typeParam) = 0;
        };
    }
}
