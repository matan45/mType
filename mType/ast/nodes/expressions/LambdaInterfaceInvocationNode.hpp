#pragma once

#include "../../ASTNode.hpp"
#include "LambdaNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>
#include <memory>
#include <string>

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
        bool isParameterTypeCompatible(value::ValueType interfaceType, value::ValueType lambdaType) const;

        // Memory safety
        bool isLambdaValid() const { return !lambdaNode.expired(); }
        void invalidateLambda() { lambdaNode.reset(); } // For cleanup scenarios
    };
}