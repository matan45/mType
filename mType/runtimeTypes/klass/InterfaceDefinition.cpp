#include "InterfaceDefinition.hpp"
#include "ClassDefinition.hpp"
#include "MethodDefinition.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include "../../value/LambdaValue.hpp"

namespace runtimeTypes::klass
{
    std::shared_ptr<ClassDefinition> InterfaceDefinition::createLambdaImplementation(
        ast::nodes::expressions::LambdaNode* lambda) const
    {
        if (!isFunctionalInterface()) {
            return nullptr; // Can only convert lambdas to functional interfaces
        }

        const MethodSignature* samMethod = getFunctionalMethod();
        if (!samMethod) {
            return nullptr;
        }

        // Create anonymous class name
        std::string className = "$Lambda$" + getName() + "$" + std::to_string(reinterpret_cast<uintptr_t>(lambda));

        // Create class definition
        auto classDefinition = std::make_shared<ClassDefinition>(className);

        // Convert lambda parameters to match interface method signature
        std::vector<std::pair<std::string, ValueType>> methodParams;
        std::vector<std::pair<std::string, Value>> methodArgs; // Empty for now
        const auto& lambdaParams = lambda->getParameters();

        // Ensure parameter count matches
        if (lambdaParams.size() != samMethod->parameters.size()) {
            return nullptr; // Parameter count mismatch
        }

        // Map lambda parameters to interface method parameters using actual interface types
        const auto& interfaceParams = samMethod->parameters;
        for (size_t i = 0; i < lambdaParams.size() && i < interfaceParams.size(); ++i) {
            const auto& lambdaParam = lambdaParams[i];
            const auto& interfaceParam = interfaceParams[i];

            // Convert GenericType to ValueType - need proper type mapping
            value::ValueType paramType = convertGenericTypeToValueType(interfaceParam.second);
            methodParams.emplace_back(lambdaParam.name, paramType);
        }

        // Create method definition that wraps the lambda
        value::ValueType returnType = convertGenericTypeToValueType(samMethod->returnType);

        // Create the specialized lambda interface invocation node
        std::vector<value::ValueType> paramTypes;
        for (const auto& [paramName, paramType] : methodParams) {
            paramTypes.push_back(paramType);
        }

        // Convert raw lambda pointer to shared_ptr for memory safety
        // NOTE: This assumes the lambda is managed elsewhere and we're creating a shared ownership
        auto lambdaSharedPtr = std::shared_ptr<ast::nodes::expressions::LambdaNode>(lambda, [](ast::nodes::expressions::LambdaNode*){
            // Custom deleter that does nothing - lambda lifetime is managed by AST
            // This prevents double deletion while allowing shared_ptr semantics
        });

        auto lambdaInvocationNode = std::make_shared<ast::nodes::expressions::LambdaInterfaceInvocationNode>(
            lambdaSharedPtr,
            std::vector<std::shared_ptr<ast::ASTNode>>(), // Arguments will be provided at runtime
            getName(),
            samMethod->name,
            paramTypes,
            returnType
        );

        auto methodDef = std::make_shared<MethodDefinition>(
            samMethod->name,
            returnType,
            methodParams,
            methodArgs,
            lambdaInvocationNode, // Use our specialized node as the method body
            false
        );

        // Store the lambda node with memory-safe shared ownership
        methodDef->setLambdaNode(lambdaSharedPtr);

        // TODO: CRITICAL - Complete lambda-to-interface implementation (Priority: HIGH)
        // TIMELINE: Should be completed within 1-2 sprints
        //
        // MISSING COMPONENTS:
        // 1. Create a specialized AST node (LambdaInterfaceInvocationNode) that:
        //    - Handles parameter mapping from interface method to lambda
        //    - Creates proper LambdaValue with runtime evaluation context
        //    - Manages type conversion between interface types and lambda types
        //
        // 2. Update the interpreter to recognize methods with lambda implementations:
        //    - Check MethodDefinition::hasLambdaNode() during method invocation
        //    - Create LambdaValue with current evaluation context
        //    - Invoke lambda with proper parameter mapping
        //
        // 3. Memory safety improvements:
        //    - Ensure lambda node lifetime is managed properly
        //    - Add weak_ptr mechanism if circular references become an issue
        //    - Implement proper cleanup when interface instances are destroyed
        //
        // CURRENT STATE: Interface creation works, but method invocation will fail
        // RISK: Runtime errors when calling lambda-backed interface methods

        // Add method to class
        classDefinition->addMethod(methodDef);

        // Mark class as implementing this interface
        classDefinition->addImplementedInterface(getName());

        return classDefinition;
    }

    value::ValueType InterfaceDefinition::convertGenericTypeToValueType(std::shared_ptr<ast::GenericType> genericType) const
    {
        if (!genericType) {
            return value::ValueType::VOID;
        }

        std::string typeName = genericType->getBaseTypeName();

        // Convert common type names to ValueType
        if (typeName == "int" || typeName == "Int") {
            return value::ValueType::INT;
        } else if (typeName == "float" || typeName == "Float") {
            return value::ValueType::FLOAT;
        } else if (typeName == "bool" || typeName == "Bool") {
            return value::ValueType::BOOL;
        } else if (typeName == "string" || typeName == "String") {
            return value::ValueType::STRING;
        } else if (typeName == "void" || typeName == "Void") {
            return value::ValueType::VOID;
        } else {
            // Default to object for complex types
            return value::ValueType::OBJECT;
        }
    }
}
