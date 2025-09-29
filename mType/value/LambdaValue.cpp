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
#include "../evaluator/utils/ValueConverter.hpp"
#include <set>
#include <iostream>

namespace value
{
    LambdaValue::LambdaValue(ast::nodes::expressions::LambdaNode* node,
                           std::shared_ptr<evaluator::base::EvaluationContext> context)
        : lambdaNode(node), capturedContext(context), isInterfaceImplementation(false)
    {
        if (!node) {
            throw errors::RuntimeException("Lambda node cannot be null");
        }
        if (!context) {
            throw errors::RuntimeException("Evaluation context cannot be null");
        }

        // Capture 'this' instance if available (for class context lambdas)
        capturedThisInstance = context->getCurrentInstance();

        // Capture the current environment for closure support
        captureCurrentEnvironment();
    }

    Value LambdaValue::invoke(const std::vector<Value>& arguments,
                             std::shared_ptr<evaluator::base::EvaluationContext> callContext)
    {

        // Enhanced lambda state validation with detailed error messages
        if (!lambdaNode) {
            std::string interfaceInfo = isImplementingInterface() ?
                " (implementing interface '" + implementedInterface + "', method '" + implementedMethod + "')" : "";
            throw errors::RuntimeException("Lambda node is null - invalid lambda state" + interfaceInfo);
        }
        if (!capturedContext) {
            std::string interfaceInfo = isImplementingInterface() ?
                " (implementing interface '" + implementedInterface + "', method '" + implementedMethod + "')" : "";
            throw errors::RuntimeException("Captured context is null - invalid lambda state" + interfaceInfo);
        }
        auto capturedEnv = capturedContext->getEnvironment();
        if (!capturedEnv) {
            throw errors::RuntimeException("Captured environment is null - invalid lambda state");
        }

        // Validate argument count and basic types
        validateArguments(arguments);

        // Create a new environment that inherits from the captured context
        auto lambdaEnv = std::make_shared<environment::Environment>(*capturedEnv);
        auto lambdaContext = std::make_shared<evaluator::base::EvaluationContext>(lambdaEnv);

        // Preserve important context from the original captured context
        // Use the captured 'this' instance (which was captured at lambda creation time)
        lambdaContext->setCurrentInstance(capturedThisInstance);
        lambdaContext->setInStaticMethod(capturedContext->isInStaticMethodContext());
        lambdaContext->setCurrentMethod(capturedContext->getCurrentMethod());
        lambdaContext->setGenericTypeBindings(capturedContext->getGenericTypeBindings());

        // Restore captured variables to lambda scope
        for (const auto& [name, captured] : capturedVariables) {
            // Get the current value based on capture strategy
            Value currentValue = captured.getValue();

            // Check if reference is still valid (for reference captures)
            if (!captured.isCapturedByValue && std::holds_alternative<std::monostate>(currentValue)) {
                throw errors::RuntimeException("Lambda references captured variable '" + name +
                                             "' which is no longer accessible");
            }

            lambdaEnv->declareVariable(name,
                std::make_shared<runtimeTypes::global::VariableDefinition>(name, captured.originalType, currentValue));
        }

        // Bind parameters to arguments
        const auto& parameters = lambdaNode->getParameters();
        for (size_t i = 0; i < parameters.size() && i < arguments.size(); ++i) {
            ValueType argType = evaluator::utils::ValueConverter::getValueType(arguments[i]);
            lambdaEnv->declareVariable(parameters[i].name,
                std::make_shared<runtimeTypes::global::VariableDefinition>(parameters[i].name, argType, arguments[i]));
        }

        // Execute lambda body
        auto lambdaBody = lambdaNode->getBody();
        if (!lambdaBody) {
            throw errors::RuntimeException("Lambda body is null - malformed lambda AST");
        }

        if (lambdaNode->isExpressionLambda()) {
            // Expression lambda - evaluate and return the expression

            // Create all evaluators for proper context support
            evaluator::ExpressionEvaluator exprEvaluator(lambdaContext);
            evaluator::StatementEvaluator stmtEvaluator(lambdaContext);
            evaluator::ObjectEvaluator objEvaluator(lambdaContext);

            // Set up evaluator dependencies (same as EvaluatorCoordinator)
            exprEvaluator.setStatementEvaluator(&stmtEvaluator);
            exprEvaluator.setObjectEvaluator(&objEvaluator);

            stmtEvaluator.setExpressionEvaluator(&exprEvaluator);
            stmtEvaluator.setObjectEvaluator(&objEvaluator);

            objEvaluator.setExpressionEvaluator(&exprEvaluator);
            objEvaluator.setStatementEvaluator(&stmtEvaluator);

            Value result = exprEvaluator.evaluate(lambdaBody);

            return result;
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
            objEvaluator.setStatementEvaluator(&stmtEvaluator);

            try {
                stmtEvaluator.evaluate(lambdaBody);
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
                    // Found as local/environment variable - use optimized capture
                    ValueType type = evaluator::utils::ValueConverter::getValueType(varDef->getValue());
                    addCapturedVariableOptimized(varName, varDef->getValue(), type);
                } else {
                    // Not found in environment, check if it's a class field
                    auto currentInstance = capturedContext->getCurrentInstance();
                    if (currentInstance) {
                        auto field = currentInstance->getField(varName);
                        if (field) {
                            // Found as instance field - capture its current value with optimization
                            Value fieldValue = currentInstance->getFieldValue(varName);
                            ValueType type = evaluator::utils::ValueConverter::getValueType(fieldValue);
                            addCapturedVariableOptimized(varName, fieldValue, type);
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
            std::string paramNames;
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (i > 0) paramNames += ", ";
                paramNames += parameters[i].name;
            }

            std::string message = "Lambda argument count mismatch: expected " +
                                std::to_string(parameters.size()) + " parameters (" + paramNames +
                                "), got " + std::to_string(arguments.size()) + " arguments";

            if (isImplementingInterface()) {
                message += " (implementing interface '" + implementedInterface +
                          "', method '" + implementedMethod + "')";
            }

            throw errors::RuntimeException(message);
        }

        // Enhanced type validation
        for (size_t i = 0; i < arguments.size() && i < parameters.size(); ++i) {
            ValueType argType = evaluator::utils::ValueConverter::getValueType(arguments[i]);
            const auto& paramName = parameters[i].name;

            // If parameter has explicit type information, validate it
            if (parameters[i].type) {
                // For now, we'll skip detailed type validation as it requires more complex type resolution
                // This could be enhanced in the future with proper GenericType->ValueType conversion
            }

            // Basic validation: ensure we don't pass null where it shouldn't be accepted
            if (std::holds_alternative<std::monostate>(arguments[i])) {
                // Allow null/void for parameters that might accept it
                // More sophisticated null checking could be added here
            }
        }
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

    bool LambdaValue::shouldCaptureByValue(const Value& value, ValueType type) const
    {
        // Always capture primitives by value (they're small)
        if (type == ValueType::INT || type == ValueType::FLOAT ||
            type == ValueType::BOOL || type == ValueType::VOID) {
            return true;
        }

        // Estimate the size and decide based on threshold
        size_t estimatedSize = estimateValueSize(value, type);
        return estimatedSize <= MAX_VALUE_CAPTURE_SIZE;
    }

    size_t LambdaValue::estimateValueSize(const Value& value, ValueType type) const
    {
        switch (type) {
            case ValueType::INT:
                return sizeof(int);
            case ValueType::FLOAT:
                return sizeof(float);
            case ValueType::BOOL:
                return sizeof(bool);
            case ValueType::STRING:
                if (std::holds_alternative<std::string>(value)) {
                    const auto& str = std::get<std::string>(value);
                    return str.size() + sizeof(std::string); // String data + overhead
                }
                return sizeof(std::string);
            case ValueType::OBJECT:
                // Objects are generally large - estimate conservatively
                return LARGE_OBJECT_THRESHOLD;
            case ValueType::ARRAY:
                // Arrays can be very large - estimate conservatively
                return LARGE_OBJECT_THRESHOLD;
            default:
                return LARGE_OBJECT_THRESHOLD; // Conservative estimate for unknown types
        }
    }

    void LambdaValue::addCapturedVariableOptimized(const std::string& name, const Value& value, ValueType type)
    {
        bool captureByValue = shouldCaptureByValue(value, type);

        if (captureByValue) {
            // Small values - capture by value for performance
            capturedVariables.emplace(name, CapturedVariable(name, value, type));
        } else {
            // Large values - capture by reference to save memory
            addCapturedVariableByReference(name, value, type);
        }
    }

    void LambdaValue::addCapturedVariableByReference(const std::string& name, const Value& value, ValueType type)
    {
        // Try to find the variable definition in the current environment
        auto environment = capturedContext->getEnvironment();
        auto varDef = environment->findVariable(name);

        if (varDef) {
            // Create weak reference to the variable definition
            std::weak_ptr<runtimeTypes::global::VariableDefinition> weakRef = varDef;
            capturedVariables.emplace(name, CapturedVariable(name, weakRef, type));
        } else {
            // Fallback: if we can't find the variable definition, capture by value
            // This can happen with temporary values or complex expressions
            capturedVariables.emplace(name, CapturedVariable(name, value, type));
        }
    }

    bool LambdaValue::isValid() const
    {
        // Check basic state
        if (!lambdaNode || !capturedContext) {
            return false;
        }

        // Check if captured environment is still valid
        auto capturedEnv = capturedContext->getEnvironment();
        if (!capturedEnv) {
            return false;
        }

        // Check captured variables (for reference captures)
        for (const auto& [name, captured] : capturedVariables) {
            if (!captured.isCapturedByValue) {
                // Check if weak reference is still valid
                Value currentValue = captured.getValue();
                if (std::holds_alternative<std::monostate>(currentValue)) {
                    return false; // Reference expired
                }
            }
        }

        return true;
    }

    std::string LambdaValue::getValidationStatus() const
    {
        std::stringstream ss;
        ss << "LambdaValue(";

        if (!lambdaNode) {
            ss << "lambdaNode=NULL, ";
        } else {
            ss << "lambdaNode=valid, ";
        }

        if (!capturedContext) {
            ss << "capturedContext=NULL, ";
        } else {
            auto capturedEnv = capturedContext->getEnvironment();
            ss << "capturedContext=" << (capturedEnv ? "valid" : "invalid") << ", ";
        }

        ss << "capturedVars=" << capturedVariables.size();

        // Check captured variables
        size_t expiredRefs = 0;
        for (const auto& [name, captured] : capturedVariables) {
            if (!captured.isCapturedByValue) {
                Value currentValue = captured.getValue();
                if (std::holds_alternative<std::monostate>(currentValue)) {
                    expiredRefs++;
                }
            }
        }

        if (expiredRefs > 0) {
            ss << ", expiredRefs=" << expiredRefs;
        }

        if (isImplementingInterface()) {
            ss << ", interface=" << implementedInterface << "." << implementedMethod;
        }

        ss << ")";
        return ss.str();
    }

    void LambdaValue::validateState() const
    {
        if (!lambdaNode) {
            std::string interfaceInfo = isImplementingInterface() ?
                " (implementing interface '" + implementedInterface + "', method '" + implementedMethod + "')" : "";
            throw errors::RuntimeException("Lambda node is null - invalid lambda state" + interfaceInfo);
        }

        if (!capturedContext) {
            std::string interfaceInfo = isImplementingInterface() ?
                " (implementing interface '" + implementedInterface + "', method '" + implementedMethod + "')" : "";
            throw errors::RuntimeException("Captured context is null - invalid lambda state" + interfaceInfo);
        }

        auto capturedEnv = capturedContext->getEnvironment();
        if (!capturedEnv) {
            std::string interfaceInfo = isImplementingInterface() ?
                " (implementing interface '" + implementedInterface + "', method '" + implementedMethod + "')" : "";
            throw errors::RuntimeException("Captured environment is null - invalid lambda state" + interfaceInfo);
        }

        // Check for expired captured variable references
        for (const auto& [name, captured] : capturedVariables) {
            if (!captured.isCapturedByValue) {
                Value currentValue = captured.getValue();
                if (std::holds_alternative<std::monostate>(currentValue)) {
                    throw errors::RuntimeException("Lambda references captured variable '" + name +
                                                 "' which is no longer accessible");
                }
            }
        }
    }
}