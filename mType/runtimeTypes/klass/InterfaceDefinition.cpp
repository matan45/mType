#include "InterfaceDefinition.hpp"
#include "ClassDefinition.hpp"
#include "MethodDefinition.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
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
        auto methodDef = std::make_shared<MethodDefinition>(
            samMethod->name,
            returnType, // Use the interface method's return type
            methodParams,
            methodArgs,
            nullptr, // We'll set a custom body that invokes the lambda
            false    // Not static
        );

        // Store reference to the lambda for later invocation
        // This is a simplified approach - in a full implementation,
        // we would create a method body that properly invokes the lambda
        // methodDef->setLambdaImplementation(lambda);  // Commented out for now

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
