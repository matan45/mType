#include "ExpressionEvaluator.hpp"
#include "Evaluator.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/MathException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <sstream>
#include <cmath>

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes;
    using namespace runtimeTypes::global;
    using namespace runtimeTypes::klass;

    ExpressionEvaluator::ExpressionEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator)
    {
    }

    Value ExpressionEvaluator::evaluateIntegerNode(IntegerNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateFloatNode(FloatNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateStringNode(StringNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateBoolNode(BoolNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateNullNode(NullNode* node)
    {
        return nullptr;
    }

    Value ExpressionEvaluator::evaluateVariableNode(VariableNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        auto varDef = env->findVariable(node->getName());
        
        if (!varDef) {
            throw UndefinedException("Undefined variable: " + node->getName(), node->getLocation());
        }
        
        return varDef->getValue();
    }

    Value ExpressionEvaluator::evaluateBinaryExpNode(BinaryExpNode* node)
    {
        Value left = mainEvaluator->evaluate(node->getLeft());
        TokenType op = node->getOperator();
        
        // Handle short-circuit evaluation for logical operators
        if (op == TokenType::AND) {
            if (!isTruthy(left)) {
                return false;
            }
            Value right = mainEvaluator->evaluate(node->getRight());
            return isTruthy(right);
        }
        else if (op == TokenType::OR) {
            if (isTruthy(left)) {
                return true;
            }
            Value right = mainEvaluator->evaluate(node->getRight());
            return isTruthy(right);
        }
        
        Value right = mainEvaluator->evaluate(node->getRight());
        
        // Handle different operator categories
        switch (op) {
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::MULTIPLY:
            case TokenType::DIVIDE:
            case TokenType::MODULO:
                return evaluateArithmetic(left, right, op);
                
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
            case TokenType::LESS:
            case TokenType::LESS_EQUALS:
            case TokenType::GREATER:
            case TokenType::GREATER_EQUALS:
                return evaluateComparison(left, right, op);
                
            default:
                throw TypeException("Unknown binary operator", node->getLocation());
        }
    }

    Value ExpressionEvaluator::evaluateTernaryExpNode(TernaryExpNode* node)
    {
        Value condition = mainEvaluator->evaluate(node->getCondition());
        
        if (isTruthy(condition)) {
            return mainEvaluator->evaluate(node->getTrueExpression());
        } else {
            return mainEvaluator->evaluate(node->getFalseExpression());
        }
    }

    Value ExpressionEvaluator::evaluateUnaryExpNode(UnaryExpNode* node)
    {
        TokenType op = node->getOperator();
        
        // Handle increment and decrement operators (require variable operand)
        if (op == TokenType::INCREMENT || op == TokenType::DECREMENT) {
            // For increment/decrement, we need to modify the variable, so we need a VariableNode
            auto varNode = dynamic_cast<VariableNode*>(node->getOperand());
            if (!varNode) {
                throw TypeException("Increment/decrement operators can only be applied to variables", node->getLocation());
            }
            
            auto env = mainEvaluator->getEnvironment();
            auto varDef = env->findVariable(varNode->getName());
            if (!varDef) {
                throw UndefinedException("Undefined variable: " + varNode->getName(), node->getLocation());
            }
            
            Value currentValue = varDef->getValue();
            
            // Check if it's a numeric type
            if (!std::holds_alternative<int>(currentValue) && !std::holds_alternative<float>(currentValue)) {
                throw TypeException("Increment/decrement operators can only be applied to numeric values", node->getLocation());
            }
            
            Value newValue;
            Value returnValue;
            
            // Calculate new value based on operator
            if (std::holds_alternative<int>(currentValue)) {
                int intVal = std::get<int>(currentValue);
                int newIntVal = (op == TokenType::INCREMENT) ? intVal + 1 : intVal - 1;
                newValue = newIntVal;
                
                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? intVal : newIntVal;
            } else {
                float floatVal = std::get<float>(currentValue);
                float newFloatVal = (op == TokenType::INCREMENT) ? floatVal + 1.0f : floatVal - 1.0f;
                newValue = newFloatVal;
                
                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? floatVal : newFloatVal;
            }
            
            // Update the variable
            varDef->setValue(newValue);
            return returnValue;
        }
        
        // Handle other unary operators (evaluate operand first)
        Value operand = mainEvaluator->evaluate(node->getOperand());
        
        switch (op) {
            case TokenType::MINUS:
                if (std::holds_alternative<int>(operand)) {
                    return -std::get<int>(operand);
                }
                if (std::holds_alternative<float>(operand)) {
                    return -std::get<float>(operand);
                }
                throw TypeException("Cannot apply unary minus to non-numeric value", SourceLocation{});
                
            case TokenType::PLUS:
                if (std::holds_alternative<int>(operand) || std::holds_alternative<float>(operand)) {
                    return operand; // Unary plus returns the value as-is
                }
                throw TypeException("Cannot apply unary plus to non-numeric value", SourceLocation{});
                
            case TokenType::NOT:
                return !isTruthy(operand);
                
            default:
                throw TypeException("Unknown unary operator", SourceLocation{});
        }
    }

    Value ExpressionEvaluator::evaluateFunctionCallNode(FunctionCallNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // First check if it's a native function
        auto nativeRegistry = env->getNativeRegistry();
        if (nativeRegistry->hasNativeFunction(node->getFunctionName())) {
            auto nativeFunc = nativeRegistry->findNativeFunction(node->getFunctionName());
            
            // Evaluate arguments
            std::vector<Value> args;
            for (auto& argNode : node->getArguments()) {
                args.push_back(mainEvaluator->evaluate(argNode.get()));
            }
            
            // Call native function
            return nativeFunc(args);
        }
        
        // Check for user-defined function
        auto funcDef = env->findFunction(node->getFunctionName());
        
        if (!funcDef) {
            throw UndefinedException("Undefined function: " + node->getFunctionName(), node->getLocation());
        }
        
        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        // Check parameter count
        if (args.size() != funcDef->getParameters().size()) {
            throw ArgumentException("Function '" + node->getFunctionName() + 
                                  "' expects " + std::to_string(funcDef->getParameters().size()) +
                                  " arguments, got " + std::to_string(args.size()),
                                  node->getLocation());
        }
        
        // Create new scope for function execution
        env->enterScope(node->getFunctionName(), ScopeType::FUNCTION);
        
        // Bind parameters to arguments
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = funcDef->getParameters()[i];
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,   // parameter name
                param.second,  // parameter type
                args[i],       // argument value
                false          // not final
            );
            env->declareVariable(param.first, varDef);
        }
        
        // Execute function body
        Value result = std::monostate{};
        try {
            mainEvaluator->evaluate(funcDef->getBody());
            
            // Get return value if function returned
            if (mainEvaluator->shouldReturn()) {
                result = mainEvaluator->getReturnValue();
                mainEvaluator->setReturned(false);
            }
        }
        catch (...) {
            env->exitScope();
            throw;
        }
        
        env->exitScope();
        return result;
    }

    Value ExpressionEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot access member of null object", node->getLocation());
        }
        
        // Get object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot access member of non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        auto field = object->getField(node->getMemberName());
        
        if (!field) {
            throw UndefinedException("Undefined field: " + node->getMemberName(), node->getLocation());
        }
        
        return field->getValue();
    }

    Value ExpressionEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot call method on null object", node->getLocation());
        }
        
        // Get object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot call method on non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        
        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        // Delegate to ObjectEvaluator through main evaluator
        return mainEvaluator->evaluateObjectMethodCall(node);
    }

    Value ExpressionEvaluator::evaluateNewNode(NewNode* node)
    {
        // Delegate to ObjectEvaluator through main evaluator
        return mainEvaluator->evaluateObjectCreation(node);
    }

    Value ExpressionEvaluator::evaluateQualifiedNameNode(QualifiedNameNode* node)
    {
        // Delegate to NamespaceEvaluator through main evaluator
        return mainEvaluator->evaluateQualifiedNameAccess(node);
    }

    // Helper methods for binary operations
    Value ExpressionEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        // Handle string concatenation for PLUS operator
        if (op == TokenType::PLUS && 
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right))) {
            return toString(left) + toString(right);
        }
        
        // Handle numeric operations
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);
        
        if (isFloat) {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);
            
            switch (op) {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0.0f) {
                        throw MathException("Division by zero", SourceLocation{});
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0.0f) {
                        throw MathException("Modulo by zero", SourceLocation{});
                    }
                    return std::fmod(leftVal, rightVal);
                default:
                    throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        } else {
            int leftVal = toInt(left);
            int rightVal = toInt(right);
            
            switch (op) {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0) {
                        throw MathException("Division by zero", SourceLocation{});
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0) {
                        throw MathException("Modulo by zero", SourceLocation{});
                    }
                    return leftVal % rightVal;
                default:
                    throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        }
    }

    Value ExpressionEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op)
    {
        // Handle null comparisons
        if (std::holds_alternative<nullptr_t>(left) || std::holds_alternative<nullptr_t>(right)) {
            bool leftNull = std::holds_alternative<nullptr_t>(left);
            bool rightNull = std::holds_alternative<nullptr_t>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftNull == rightNull;
                case TokenType::NOT_EQUALS: return leftNull != rightNull;
                default:
                    throw TypeException("Cannot compare null with relational operators", SourceLocation{});
            }
        }
        
        // Handle string comparisons
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
            std::string leftStr = std::get<std::string>(left);
            std::string rightStr = std::get<std::string>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftStr == rightStr;
                case TokenType::NOT_EQUALS: return leftStr != rightStr;
                case TokenType::LESS: return leftStr < rightStr;
                case TokenType::LESS_EQUALS: return leftStr <= rightStr;
                case TokenType::GREATER: return leftStr > rightStr;
                case TokenType::GREATER_EQUALS: return leftStr >= rightStr;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
        
        // Handle boolean comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
            bool leftBool = std::get<bool>(left);
            bool rightBool = std::get<bool>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftBool == rightBool;
                case TokenType::NOT_EQUALS: return leftBool != rightBool;
                default:
                    throw TypeException("Cannot compare booleans with relational operators", SourceLocation{});
            }
        }
        
        // Handle numeric comparisons
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);
        
        if (isFloat) {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftVal == rightVal;
                case TokenType::NOT_EQUALS: return leftVal != rightVal;
                case TokenType::LESS: return leftVal < rightVal;
                case TokenType::LESS_EQUALS: return leftVal <= rightVal;
                case TokenType::GREATER: return leftVal > rightVal;
                case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        } else {
            int leftVal = toInt(left);
            int rightVal = toInt(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftVal == rightVal;
                case TokenType::NOT_EQUALS: return leftVal != rightVal;
                case TokenType::LESS: return leftVal < rightVal;
                case TokenType::LESS_EQUALS: return leftVal <= rightVal;
                case TokenType::GREATER: return leftVal > rightVal;
                case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
    }

    Value ExpressionEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op)
    {
        bool leftBool = isTruthy(left);
        bool rightBool = isTruthy(right);
        
        switch (op) {
            case TokenType::AND: return leftBool && rightBool;
            case TokenType::OR: return leftBool || rightBool;
            default:
                throw TypeException("Invalid logical operator", SourceLocation{});
        }
    }

    Value ExpressionEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        if (op == TokenType::PLUS) {
            return toString(left) + toString(right);
        }
        throw TypeException("Invalid string operator", SourceLocation{});
    }

    // Type coercion helpers
    bool ExpressionEvaluator::isTruthy(const Value& value)
    {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value);
        }
        if (std::holds_alternative<nullptr_t>(value)) {
            return false;
        }
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value) != 0;
        }
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value) != 0.0f;
        }
        if (std::holds_alternative<std::string>(value)) {
            return !std::get<std::string>(value).empty();
        }
        if (std::holds_alternative<std::monostate>(value)) {
            return false;
        }
        // Object instances are truthy if not null
        return true;
    }

    float ExpressionEvaluator::toFloat(const Value& value)
    {
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value);
        }
        if (std::holds_alternative<int>(value)) {
            return static_cast<float>(std::get<int>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? 1.0f : 0.0f;
        }
        throw TypeException("Cannot convert to float", SourceLocation{});
    }

    int ExpressionEvaluator::toInt(const Value& value)
    {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        }
        if (std::holds_alternative<float>(value)) {
            return static_cast<int>(std::get<float>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? 1 : 0;
        }
        throw TypeException("Cannot convert to int", SourceLocation{});
    }

    std::string ExpressionEvaluator::toString(const Value& value)
    {
        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }
        if (std::holds_alternative<int>(value)) {
            return std::to_string(std::get<int>(value));
        }
        if (std::holds_alternative<float>(value)) {
            return std::to_string(std::get<float>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "true" : "false";
        }
        if (std::holds_alternative<nullptr_t>(value)) {
            return "null";
        }
        if (std::holds_alternative<std::monostate>(value)) {
            return "";
        }
        // For objects, return a string representation
        return "[object]";
    }
}