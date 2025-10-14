#pragma once

#include "../../ASTNode.hpp"
#include "LambdaNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace ast::nodes::expressions
{
    /**
     * Specialized AST node for handling lambda-to-interface method invocation
     * This node bridges the gap between interface method calls and lambda execution
     */
    class LambdaInterfaceInvocationNode : public ast::ASTNode
    {
    private:
        std::weak_ptr<LambdaNode> lambdaNode;                   // The lambda to invoke (weak_ptr for memory safety)
        std::vector<std::shared_ptr<ast::ASTNode>> arguments;   // Method call arguments
        std::string interfaceName;                                // Interface being implemented
        std::string methodName;                                   // Method being invoked

        // Type mapping information
        std::vector<value::ValueType> interfaceParameterTypes;    // Expected interface types
        value::ValueType interfaceReturnType;                     // Expected return type

        // Generic type support
        std::unordered_map<std::string, value::ValueType> genericTypeBindings; // T -> int, etc.

        // Lifecycle management
        mutable bool markedAsInvalid = false;
        mutable size_t accessCount = 0;

    public:
        LambdaInterfaceInvocationNode(
            std::shared_ptr<LambdaNode> lambda,
            const std::vector<std::shared_ptr<ast::ASTNode>>& args,
            const std::string& interface,
            const std::string& method,
            const std::vector<value::ValueType>& paramTypes,
            value::ValueType returnType
        );

        // Core AST node interface
        value::Value accept(ast::ASTVisitor<value::Value>& visitor) override;

        // Utility method for debugging
        std::string toString() const;

        // Accessors
        std::shared_ptr<LambdaNode> getLambdaNode() const { return lambdaNode.lock(); }
        const std::vector<std::shared_ptr<ast::ASTNode>>& getArguments() const { return arguments; }
        const std::string& getInterfaceName() const { return interfaceName; }
        const std::string& getMethodName() const { return methodName; }
        const std::vector<value::ValueType>& getInterfaceParameterTypes() const { return interfaceParameterTypes; }
        value::ValueType getInterfaceReturnType() const { return interfaceReturnType; }

        // Type validation
        bool validateParameterTypes(const std::vector<value::Value>& actualArgs) const;
        bool validateParameterTypes(const std::vector<value::Value>& actualArgs, std::string* errorMessage) const;
        bool isParameterTypeCompatible(value::ValueType interfaceType, value::ValueType lambdaType) const;

        // Return type validation
        bool validateReturnType(const value::Value& returnValue) const;
        bool validateReturnType(const value::Value& returnValue, std::string* errorMessage) const;
        bool isReturnTypeCompatible(value::ValueType actualReturnType) const;

        // Generic type support
        void setGenericTypeBinding(const std::string& typeParam, value::ValueType actualType);
        value::ValueType resolveGenericType(const std::string& typeParam) const;
        bool hasGenericTypeBindings() const { return !genericTypeBindings.empty(); }
        const std::unordered_map<std::string, value::ValueType>& getGenericTypeBindings() const { return genericTypeBindings; }

        // Memory safety and lifecycle management
        bool isLambdaValid() const { return !lambdaNode.expired(); }
        void invalidateLambda() { lambdaNode.reset(); } // For cleanup scenarios

        // Enhanced lifecycle management
        bool isLambdaAccessible() const;
        void markAsInvalid();
        std::string getLambdaLifecycleStatus() const;

        // Cleanup and resource management
        void cleanup();
        bool needsCleanup() const;

        std::unique_ptr<ASTNode> clone() const override;

    private:
        // Helper methods
        std::string getValueTypeName(value::ValueType type) const;
    };
}