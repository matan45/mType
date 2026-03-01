#include "InterfaceDefinition.hpp"
#include "ClassDefinition.hpp"
#include "MethodDefinition.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"

namespace runtimeTypes::klass
{
    std::string InterfaceDefinition::createAnonymousClassName(ast::nodes::expressions::LambdaNode* lambda) const
    {
        return "$Lambda$" + getName() + "$" + std::to_string(reinterpret_cast<uintptr_t>(lambda));
    }

    std::vector<std::pair<std::string, value::ValueType>> InterfaceDefinition::mapLambdaParameters(
        ast::nodes::expressions::LambdaNode* lambda,
        const MethodSignature* samMethod) const
    {
        std::vector<std::pair<std::string, value::ValueType>> methodParams;
        const auto& lambdaParams = lambda->getParameters();
        const auto& interfaceParams = samMethod->parameters;

        for (size_t i = 0; i < lambdaParams.size() && i < interfaceParams.size(); ++i) {
            const auto& lambdaParam = lambdaParams[i];
            const auto& interfaceParam = interfaceParams[i];

            // Convert UnifiedType to ValueType
            value::ValueType paramType = convertUnifiedTypeToValueType(interfaceParam.second);
            methodParams.emplace_back(lambdaParam.name, paramType);
        }

        return methodParams;
    }

    std::shared_ptr<ast::nodes::expressions::LambdaInterfaceInvocationNode> InterfaceDefinition::createLambdaInvocationNode(
        std::shared_ptr<ast::nodes::expressions::LambdaNode> lambdaSharedPtr,
        const MethodSignature* samMethod,
        const std::vector<std::pair<std::string, value::ValueType>>& methodParams) const
    {
        value::ValueType returnType = convertUnifiedTypeToValueType(samMethod->returnType);

        std::vector<value::ValueType> paramTypes;
        for (const auto& [paramName, paramType] : methodParams) {
            paramTypes.push_back(paramType);
        }

        return std::make_shared<ast::nodes::expressions::LambdaInterfaceInvocationNode>(
            lambdaSharedPtr,
            std::vector<std::shared_ptr<ast::ASTNode>>(), // Arguments will be provided at runtime
            getName(),
            samMethod->name,
            paramTypes,
            returnType
        );
    }

    std::shared_ptr<ClassDefinition> InterfaceDefinition::createLambdaImplementation(
        ast::nodes::expressions::LambdaNode* lambda,
        const std::string& fullInterfaceName) const
    {
        if (!isFunctionalInterface()) {
            return nullptr; // Can only convert lambdas to functional interfaces
        }

        const MethodSignature* samMethod = getFunctionalMethod();
        if (!samMethod) {
            return nullptr;
        }

        // Create anonymous class name
        std::string className = createAnonymousClassName(lambda);

        // Create class definition
        auto classDefinition = std::make_shared<ClassDefinition>(className);

        // Ensure parameter count matches
        if (lambda->getParameters().size() != samMethod->parameters.size()) {
            return nullptr; // Parameter count mismatch
        }

        // Map lambda parameters to interface method parameters
        auto methodParams = mapLambdaParameters(lambda, samMethod);

        // Non-owning shared_ptr: the LambdaNode is owned by the AST (parser's unique_ptr).
        // No-op deleter prevents double-free. Receivers store this as weak_ptr,
        // so they safely detect when the AST is destroyed via weak_ptr::expired().
        auto lambdaSharedPtr = std::shared_ptr<ast::nodes::expressions::LambdaNode>(lambda, [](ast::nodes::expressions::LambdaNode*){});

        // Create lambda invocation node
        auto lambdaInvocationNode = createLambdaInvocationNode(lambdaSharedPtr, samMethod, methodParams);

        // Create method definition that wraps the lambda
        value::ValueType returnType = convertUnifiedTypeToValueType(samMethod->returnType);
        auto methodDef = std::make_shared<MethodDefinition>(
            samMethod->name,
            returnType,
            methodParams,
            lambdaInvocationNode,
            false
        );

        // Store the lambda node with memory-safe shared ownership
        methodDef->setLambdaNode(lambdaSharedPtr);

        // Add method to class
        classDefinition->addMethod(methodDef);

        // Mark class as implementing this interface
        // Use full interface name (with generics) if provided, otherwise use base name
        std::string interfaceToImplement = fullInterfaceName.empty() ? getName() : fullInterfaceName;
        classDefinition->addImplementedInterface(interfaceToImplement);

        return classDefinition;
    }

    value::ValueType InterfaceDefinition::convertUnifiedTypeToValueType(const ::types::UnifiedTypePtr& type) const
    {
        if (!type) {
            return value::ValueType::VOID;
        }
        return type->toValueType();
    }
}
