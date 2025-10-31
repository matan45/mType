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

            // Convert GenericType to ValueType
            value::ValueType paramType = convertGenericTypeToValueType(interfaceParam.second);
            methodParams.emplace_back(lambdaParam.name, paramType);
        }

        return methodParams;
    }

    std::shared_ptr<ast::nodes::expressions::LambdaInterfaceInvocationNode> InterfaceDefinition::createLambdaInvocationNode(
        std::shared_ptr<ast::nodes::expressions::LambdaNode> lambdaSharedPtr,
        const MethodSignature* samMethod,
        const std::vector<std::pair<std::string, value::ValueType>>& methodParams) const
    {
        value::ValueType returnType = convertGenericTypeToValueType(samMethod->returnType);

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

        // Convert raw lambda pointer to shared_ptr for memory safety
        auto lambdaSharedPtr = std::shared_ptr<ast::nodes::expressions::LambdaNode>(lambda, [](ast::nodes::expressions::LambdaNode* ptr){
            // Custom deleter that does nothing - lambda lifetime is managed by AST
            (void)ptr; // Silence unused parameter warning
        });

        // Create lambda invocation node
        auto lambdaInvocationNode = createLambdaInvocationNode(lambdaSharedPtr, samMethod, methodParams);

        // Create method definition that wraps the lambda
        value::ValueType returnType = convertGenericTypeToValueType(samMethod->returnType);
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

    value::ValueType InterfaceDefinition::convertGenericTypeToValueType(std::shared_ptr<ast::GenericType> genericType) const
    {
        if (!genericType) {
            return value::ValueType::VOID;
        }

        std::string typeName = genericType->getBaseTypeName();

        // Handle generic type parameters (e.g., T, K, V)
        if (genericType->isGenericParameter()) {
            // For generic parameters, we need to resolve them at runtime
            // For now, we'll treat them as objects since we can't resolve them at this point
            // A more sophisticated implementation would track generic type bindings
            return value::ValueType::OBJECT;
        }

        // Handle parameterized generic types (e.g., List<T>, Map<K,V>)
        if (genericType->isParameterized()) {
            const auto& genericParams = genericType->getTypeArguments();

            // Handle common collection types
            if (typeName == "List" || typeName == "Array") {
                return value::ValueType::ARRAY;
            } else if (typeName == "Map" || typeName == "HashMap") {
                return value::ValueType::OBJECT; // Maps are complex objects
            } else if (typeName == "Set" || typeName == "HashSet") {
                return value::ValueType::OBJECT; // Sets are complex objects
            } else {
                // Other parameterized types default to object
                return value::ValueType::OBJECT;
            }
        }

        // Convert primitive type names to ValueType
        if (typeName == "int" || typeName == "Int" || typeName == "Integer") {
            return value::ValueType::INT;
        } else if (typeName == "float" || typeName == "Float" || typeName == "Double" || typeName == "double") {
            return value::ValueType::FLOAT;
        } else if (typeName == "bool" || typeName == "Bool" || typeName == "Boolean" || typeName == "boolean") {
            return value::ValueType::BOOL;
        } else if (typeName == "string" || typeName == "String") {
            return value::ValueType::STRING;
        } else if (typeName == "void" || typeName == "Void") {
            return value::ValueType::VOID;
        } else if (typeName == "Object" || typeName == "object") {
            return value::ValueType::OBJECT;
        } else if (typeName == "Function" || typeName == "function" || typeName == "Lambda" || typeName == "lambda") {
            return value::ValueType::LAMBDA;
        } else {
            // For custom classes and complex types, default to object
            return value::ValueType::OBJECT;
        }
    }
}
