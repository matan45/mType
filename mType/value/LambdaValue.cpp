#include "LambdaValue.hpp"
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../environment/Environment.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../evaluator/ExpressionEvaluator.hpp"
#include "../evaluator/StatementEvaluator.hpp"
#include "../evaluator/ObjectEvaluator.hpp"
#include "../exception/ReturnException.hpp"
#include <set>

namespace value
{
    LambdaValue::LambdaValue(ast::nodes::expressions::LambdaNode* node,
                           std::shared_ptr<evaluator::base::EvaluationContext> context)
        : lambdaNode(node), capturedContext(context), isInterfaceImplementation(false)
    {
        // Capture 'this' instance if available (for class context lambdas)
        capturedThisInstance = context->getCurrentInstance();

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

        // Preserve important context from the original captured context
        // Use the captured 'this' instance (which was captured at lambda creation time)
        lambdaContext->setCurrentInstance(capturedThisInstance);
        lambdaContext->setInStaticMethod(capturedContext->isInStaticMethodContext());
        lambdaContext->setCurrentMethod(capturedContext->getCurrentMethod());
        lambdaContext->setGenericTypeBindings(capturedContext->getGenericTypeBindings());

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
            // Create all evaluators for proper context support
            evaluator::ExpressionEvaluator exprEvaluator(lambdaContext);
            evaluator::StatementEvaluator stmtEvaluator(lambdaContext);
            evaluator::ObjectEvaluator objEvaluator(lambdaContext);

            // Set up evaluator dependencies
            exprEvaluator.setStatementEvaluator(&stmtEvaluator);
            exprEvaluator.setObjectEvaluator(&objEvaluator);

            return exprEvaluator.evaluate(lambdaNode->getBody());
        } else {
            // Block lambda - execute statements and get return value
            // Create all three evaluators and link them properly
            evaluator::StatementEvaluator stmtEvaluator(lambdaContext);
            evaluator::ExpressionEvaluator exprEvaluator(lambdaContext);
            evaluator::ObjectEvaluator objEvaluator(lambdaContext);

            // Set up evaluator dependencies (same as EvaluatorCoordinator)
            exprEvaluator.setStatementEvaluator(&stmtEvaluator);
            exprEvaluator.setObjectEvaluator(&objEvaluator);

            stmtEvaluator.setExpressionEvaluator(&exprEvaluator);
            stmtEvaluator.setObjectEvaluator(&objEvaluator);

            objEvaluator.setExpressionEvaluator(&exprEvaluator);

            try {
                stmtEvaluator.evaluate(lambdaNode->getBody());
            } catch (const exception::ReturnException& e) {
                // Return statements in lambdas are normal - extract the return value
                return e.returnValue;
            }

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
                    // Found as local/environment variable
                    ValueType type = getValueType(varDef->getValue());
                    addCapturedVariable(varName, varDef->getValue(), type);
                } else {
                    // Not found in environment, check if it's a class field
                    auto currentInstance = capturedContext->getCurrentInstance();
                    if (currentInstance) {
                        auto field = currentInstance->getField(varName);
                        if (field) {
                            // Found as instance field - capture its current value
                            Value fieldValue = currentInstance->getFieldValue(varName);
                            ValueType type = getValueType(fieldValue);
                            addCapturedVariable(varName, fieldValue, type);
                        }
                    }
                }
            }
        }
    }

    std::vector<std::string> LambdaValue::analyzeVariableReferences() const
    {
        std::set<std::string> referencedVars;

        // Traverse the lambda body to find all variable references
        traverseForVariables(lambdaNode->getBody(), referencedVars);

        return std::vector<std::string>(referencedVars.begin(), referencedVars.end());
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

    void LambdaValue::traverseForVariables(const ast::ASTNode* node, std::set<std::string>& variables) const
    {
        if (!node) return;

        // Check for variable nodes (variable references)
        if (auto varNode = dynamic_cast<const ast::nodes::expressions::VariableNode*>(node)) {
            variables.insert(varNode->getName());
            return;
        }

        // Handle binary operations
        if (auto binOpNode = dynamic_cast<const ast::nodes::expressions::BinaryExpNode*>(node)) {
            traverseForVariables(binOpNode->getLeft(), variables);
            traverseForVariables(binOpNode->getRight(), variables);
            return;
        }

        // Handle unary operations
        if (auto unaryNode = dynamic_cast<const ast::nodes::expressions::UnaryExpNode*>(node)) {
            traverseForVariables(unaryNode->getOperand(), variables);
            return;
        }

        // Handle block statements
        if (auto blockNode = dynamic_cast<const ast::nodes::statements::BlockNode*>(node)) {
            for (const auto& statement : blockNode->getStatements()) {
                traverseForVariables(statement.get(), variables);
            }
            return;
        }

        // Handle assignments
        if (auto assignNode = dynamic_cast<const ast::nodes::statements::AssignmentNode*>(node)) {
            traverseForVariables(assignNode->getValue(), variables);
            return;
        }

        // Handle declarations (only traverse initializer)
        if (auto declNode = dynamic_cast<const ast::nodes::statements::DeclarationNode*>(node)) {
            if (declNode->getInitializer()) {
                traverseForVariables(declNode->getInitializer(), variables);
            }
            return;
        }

        // Handle if statements
        if (auto ifNode = dynamic_cast<const ast::nodes::statements::IfNode*>(node)) {
            traverseForVariables(ifNode->getCondition(), variables);
            traverseForVariables(ifNode->getThenStatement(), variables);
            if (ifNode->hasElseStatement()) {
                traverseForVariables(ifNode->getElseStatement(), variables);
            }
            return;
        }

        // Handle while loops
        if (auto whileNode = dynamic_cast<const ast::nodes::statements::WhileNode*>(node)) {
            traverseForVariables(whileNode->getCondition(), variables);
            traverseForVariables(whileNode->getBody(), variables);
            return;
        }

        // Handle return statements
        if (auto returnNode = dynamic_cast<const ast::nodes::functions::ReturnNode*>(node)) {
            if (returnNode->hasReturnValue()) {
                traverseForVariables(returnNode->getReturnValue(), variables);
            }
            return;
        }

        // For other node types, we would need to add specific handling
        // This covers the most common cases for variable references in lambdas
    }
}