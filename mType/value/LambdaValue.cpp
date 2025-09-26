#include "LambdaValue.hpp"
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "../environment/Environment.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../evaluator/ExpressionEvaluator.hpp"
#include "../evaluator/StatementEvaluator.hpp"
#include <set>

namespace value
{
    LambdaValue::LambdaValue(ast::nodes::expressions::LambdaNode* node,
                           std::shared_ptr<evaluator::base::EvaluationContext> context)
        : lambdaNode(node), capturedContext(context), isInterfaceImplementation(false)
    {
        // Capture the current environment for closure support
        captureCurrentEnvironment();
    }

    Value LambdaValue::invoke(const std::vector<Value>& arguments,
                             std::shared_ptr<evaluator::base::EvaluationContext> callContext)
    {
        // Validate argument count and basic types
        validateArguments(arguments);

        // Create a new environment that inherits from the captured context
        auto lambdaEnv = std::make_shared<environment::Environment>(*capturedContext->getEnvironment());
        auto lambdaContext = std::make_shared<evaluator::base::EvaluationContext>(lambdaEnv);

        // Restore captured variables to lambda scope
        for (const auto& [name, captured] : capturedVariables) {
            lambdaEnv->declareVariable(name,
                std::make_shared<runtimeTypes::global::VariableDefinition>(name, captured.originalType, captured.value));
        }

        // Bind parameters to arguments
        const auto& parameters = lambdaNode->getParameters();
        for (size_t i = 0; i < parameters.size() && i < arguments.size(); ++i) {
            ValueType argType = getValueType(arguments[i]);
            lambdaEnv->declareVariable(parameters[i].name,
                std::make_shared<runtimeTypes::global::VariableDefinition>(parameters[i].name, argType, arguments[i]));
        }

        // Execute lambda body
        if (lambdaNode->isExpressionLambda()) {
            // Expression lambda - evaluate and return the expression
            evaluator::ExpressionEvaluator evaluator(lambdaContext);
            return evaluator.evaluate(lambdaNode->getBody());
        } else {
            // Block lambda - execute statements and get return value
            evaluator::StatementEvaluator evaluator(lambdaContext);
            evaluator.evaluate(lambdaNode->getBody());

            // Check if a return value was set
            if (lambdaContext->shouldReturn()) {
                return lambdaContext->getReturnValue();
            }

            // No explicit return, return void
            return std::monostate{};
        }
    }

    void LambdaValue::addCapturedVariable(const std::string& name, const Value& value, ValueType type)
    {
        capturedVariables.emplace(name, CapturedVariable(name, value, type));
    }

    bool LambdaValue::hasCapturedVariable(const std::string& name) const
    {
        return capturedVariables.find(name) != capturedVariables.end();
    }

    const CapturedVariable* LambdaValue::getCapturedVariable(const std::string& name) const
    {
        auto it = capturedVariables.find(name);
        return (it != capturedVariables.end()) ? &it->second : nullptr;
    }

    void LambdaValue::setInterfaceImplementation(const std::string& interface, const std::string& method)
    {
        implementedInterface = interface;
        implementedMethod = method;
        isInterfaceImplementation = true;
    }

    std::vector<ValueType> LambdaValue::getParameterTypes() const
    {
        std::vector<ValueType> types;
        const auto& parameters = lambdaNode->getParameters();

        for (const auto& param : parameters) {
            if (param.type) {
                // Type is specified - we would need to convert from GenericType to ValueType
                // For now, we'll use a simple heuristic based on type name
                std::string typeName = param.type->getBaseTypeName();
                if (typeName == "int" || typeName == "Int") {
                    types.push_back(ValueType::INT);
                } else if (typeName == "float" || typeName == "Float") {
                    types.push_back(ValueType::FLOAT);
                } else if (typeName == "bool" || typeName == "Bool") {
                    types.push_back(ValueType::BOOL);
                } else if (typeName == "string" || typeName == "String") {
                    types.push_back(ValueType::STRING);
                } else {
                    types.push_back(ValueType::OBJECT);
                }
            } else {
                // Type inference would happen during type checking phase
                types.push_back(ValueType::OBJECT); // Default to object for now
            }
        }

        return types;
    }

    ValueType LambdaValue::getReturnType() const
    {
        // Return type inference would be implemented during type checking
        // For now, return VOID for block lambdas and OBJECT for expression lambdas
        return lambdaNode->isExpressionLambda() ? ValueType::OBJECT : ValueType::VOID;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> LambdaValue::createInterfaceProxy() const
    {
        // This would create an anonymous class implementation of the functional interface
        // For now, return nullptr - this will be implemented when we add interface support
        return nullptr;
    }

    void LambdaValue::captureCurrentEnvironment()
    {
        // Analyze lambda body to find referenced variables
        std::vector<std::string> referencedVars = analyzeVariableReferences();

        // Capture variables from current environment
        auto environment = capturedContext->getEnvironment();
        for (const auto& varName : referencedVars) {
            // Skip parameter names (they will be bound during invocation)
            bool isParameter = false;
            for (const auto& param : lambdaNode->getParameters()) {
                if (param.name == varName) {
                    isParameter = true;
                    break;
                }
            }

            if (!isParameter) {
                auto varDef = environment->findVariable(varName);
                if (varDef) {
                    ValueType type = getValueType(varDef->getValue());
                    addCapturedVariable(varName, varDef->getValue(), type);
                }
            }
        }
    }

    std::vector<std::string> LambdaValue::analyzeVariableReferences() const
    {
        // This is a simplified implementation - in a real implementation,
        // we would traverse the AST to find all variable references
        // For now, return an empty vector (no closure capture)
        return {};
    }

    void LambdaValue::validateArguments(const std::vector<Value>& arguments) const
    {
        const auto& parameters = lambdaNode->getParameters();

        if (arguments.size() != parameters.size()) {
            throw errors::RuntimeException(
                "Lambda argument count mismatch: expected " +
                std::to_string(parameters.size()) +
                ", got " + std::to_string(arguments.size()));
        }

        // Additional type validation could be added here
    }
}