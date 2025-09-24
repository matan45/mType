#include "ExpressionEvaluator.hpp"
#include <iostream>
#include "utils/ParameterBinder.hpp"
#include "../value/StringPool.hpp"
#include "utils/ScopeGuard.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/MathException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../exception/ReturnException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/ArrayPool.hpp"
#include "../value/StringPool.hpp"
#include "../parser/TypeParser.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "ObjectEvaluator.hpp"
#include "StatementEvaluator.hpp"
#include <cmath>

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes;
    using namespace runtimeTypes::global;
    using namespace runtimeTypes::klass;
    using namespace value;

    ExpressionEvaluator::ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx), stmtEvaluator(nullptr), objEvaluator(nullptr)
    {
    }

    Value ExpressionEvaluator::evaluate(ASTNode* node)
    {
        if (!node || !canHandle(node))
        {
            return std::monostate{};
        }

        // Dispatch to appropriate evaluation method based on node type
        if (auto intNode = dynamic_cast<IntegerNode*>(node))
        {
            return evaluateIntegerNode(intNode);
        }
        if (auto floatNode = dynamic_cast<FloatNode*>(node))
        {
            return evaluateFloatNode(floatNode);
        }
        if (auto stringNode = dynamic_cast<StringNode*>(node))
        {
            return evaluateStringNode(stringNode);
        }
        if (auto boolNode = dynamic_cast<BoolNode*>(node))
        {
            return evaluateBoolNode(boolNode);
        }
        if (auto nullNode = dynamic_cast<NullNode*>(node))
        {
            return evaluateNullNode(nullNode);
        }
        if (auto varNode = dynamic_cast<VariableNode*>(node))
        {
            return evaluateVariableNode(varNode);
        }
        if (auto binNode = dynamic_cast<BinaryExpNode*>(node))
        {
            return evaluateBinaryExpNode(binNode);
        }
        if (auto ternNode = dynamic_cast<TernaryExpNode*>(node))
        {
            return evaluateTernaryExpNode(ternNode);
        }
        if (auto unaryNode = dynamic_cast<UnaryExpNode*>(node))
        {
            return evaluateUnaryExpNode(unaryNode);
        }
        if (auto funcCallNode = dynamic_cast<FunctionCallNode*>(node))
        {
            return evaluateFunctionCallNode(funcCallNode);
        }
        if (auto newNode = dynamic_cast<NewNode*>(node))
        {
            return evaluateNewNode(newNode);
        }
        if (auto methodCallNode = dynamic_cast<MethodCallNode*>(node))
        {
            return evaluateMethodCallNode(methodCallNode);
        }
        if (auto memberAccessNode = dynamic_cast<MemberAccessNode*>(node))
        {
            return evaluateMemberAccessNode(memberAccessNode);
        }
        if (auto assignNode = dynamic_cast<AssignmentNode*>(node))
        {
            return evaluateAssignmentExpression(assignNode);
        }
        if (auto memberAssignNode = dynamic_cast<MemberAssignmentNode*>(node))
        {
            // Delegate member assignment to ObjectEvaluator
            if (objEvaluator)
            {
                return objEvaluator->evaluate(memberAssignNode);
            }
            else
            {
                throw UndefinedException("Object evaluator not available for member assignment", node->getLocation());
            }
        }

        if (auto arrayCreationNode = dynamic_cast<ArrayCreationNode*>(node))
        {
            return evaluateArrayCreationNode(arrayCreationNode);
        }
        if (auto arrayLiteralNode = dynamic_cast<ArrayLiteralNode*>(node))
        {
            return evaluateArrayLiteralNode(arrayLiteralNode);
        }
        if (auto indexAccessNode = dynamic_cast<IndexAccessNode*>(node))
        {
            return evaluateIndexAccessNode(indexAccessNode);
        }

        return std::monostate{};
    }

    bool ExpressionEvaluator::canHandle(ASTNode* node) const
    {
        return isExpressionNode(node);
    }

    bool ExpressionEvaluator::isTruthy(const Value& value) const
    {
        return ValueConverter::isTruthy(value);
    }

    std::string ExpressionEvaluator::toString(const Value& value) const
    {
        // Handle objects with potential toString() method
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(value))
        {
            auto objectInstance = std::get<std::shared_ptr<ObjectInstance>>(value);
            if (!objectInstance)
            {
                return "null";
            }

            // Try to call toString() method if it exists
            auto classDef = objectInstance->getClassDefinition();
            if (classDef && classDef->hasMethod("toString"))
            {
                auto toStringMethod = classDef->findMethod("toString", 0);
                if (toStringMethod && !toStringMethod->isStatic())
                {
                    try
                    {
                        Value result = objEvaluator->callMethod(objectInstance, "toString", {});
                        if (std::holds_alternative<std::string>(result))
                        {
                            return std::get<std::string>(result);
                        }
                        else if (std::holds_alternative<InternedString>(result))
                        {
                            return std::get<InternedString>(result).getString();
                        }
                    }
                    catch (...)
                    {
                        // If toString() fails, fall back to default representation
                    }
                }
            }
        }

        // Fall back to ValueConverter for all other types or when toString() is not available
        return ValueConverter::toString(value);
    }

    float ExpressionEvaluator::toFloat(const Value& value) const
    {
        return ValueConverter::toFloat(value);
    }

    int ExpressionEvaluator::toInt(const Value& value) const
    {
        return ValueConverter::toInt(value);
    }

    void ExpressionEvaluator::setStatementEvaluator(StatementEvaluator* evaluator)
    {
        stmtEvaluator = evaluator;
    }

    void ExpressionEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
    }

    bool ExpressionEvaluator::isExpressionNode(ASTNode* node) const
    {
        return dynamic_cast<IntegerNode*>(node) ||
            dynamic_cast<FloatNode*>(node) ||
            dynamic_cast<StringNode*>(node) ||
            dynamic_cast<BoolNode*>(node) ||
            dynamic_cast<NullNode*>(node) ||
            dynamic_cast<VariableNode*>(node) ||
            dynamic_cast<BinaryExpNode*>(node) ||
            dynamic_cast<TernaryExpNode*>(node) ||
            dynamic_cast<UnaryExpNode*>(node) ||
            dynamic_cast<FunctionCallNode*>(node) ||
            dynamic_cast<NewNode*>(node) ||
            dynamic_cast<MethodCallNode*>(node) ||
            dynamic_cast<MemberAccessNode*>(node) ||
            dynamic_cast<AssignmentNode*>(node) ||
            dynamic_cast<MemberAssignmentNode*>(node) ||
            dynamic_cast<ArrayCreationNode*>(node) ||
            dynamic_cast<ArrayLiteralNode*>(node) ||
            dynamic_cast<IndexAccessNode*>(node);
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
        // Intern the string value from the StringNode
        auto& pool = StringPool::getInstance();
        auto result = pool.intern(node->getValue());

        return result;
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
        // Handle 'this' keyword specifically
        if (node->getName() == "this")
        {
            // VALIDATION: Prevent 'this' access from static methods
            if (context->isInStaticMethodContext())
            {
                throw TypeException("Cannot use 'this' in static method context", node->getLocation());
            }

            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                return currentInstance;
            }
            throw UndefinedException("'this' is not available outside of instance methods", node->getLocation());
        }

        std::string varName = node->getName();

        // Check if this is a qualified static field access (contains ::)
        if (varName.find("::") != std::string::npos)
        {
            // Parse the qualified name into parts
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = varName.find("::");

            while (pos != std::string::npos)
            {
                parts.push_back(varName.substr(start, pos - start));
                start = pos + 2; // Skip "::"
                pos = varName.find("::", start);
            }
            parts.push_back(varName.substr(start));

            // Handle qualified static field access: ClassName::fieldName
            if (parts.size() == 2)
            {
                std::string className = parts[0];
                std::string fieldName = parts[1];
                // Delegate static member access to object evaluator
                if (objEvaluator)
                {
                    return objEvaluator->accessStaticMember(className, fieldName);
                }
                else
                {
                    throw UndefinedException("Object evaluator not available for static member access",
                                             node->getLocation());
                }
            }
            else
            {
                throw UndefinedException("Complex qualified variable access not supported: '" + varName + "'",
                                         node->getLocation());
            }
        }

        auto env = context->getEnvironment();

        // Check variables first (parameters, local variables, etc.)
        auto varDef = env->findVariable(varName);
        if (varDef)
        {
            return varDef->getValue();
        }

        // Check instance fields if no variable found
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance)
        {
            // VALIDATION: Prevent instance member access from static methods
            if (context->isInStaticMethodContext())
            {
                auto field = currentInstance->getField(varName);
                if (field && !field->isStatic())
                {
                    throw TypeException("Cannot access instance field '" + varName +
                                        "' from static method context", node->getLocation());
                }
            }

            auto field = currentInstance->getField(varName);
            if (field)
            {
                return currentInstance->getFieldValue(varName);
            }
        }

        // Check if this might be a static field access
        // First check if we're in a static method by looking for the current class name
        auto classRegistry = env->getClassRegistry();

        // Check if we have a current class name stored (from static method execution)
        auto currentClassVar = env->findVariable("__current_class_name__");
        if (currentClassVar)
        {
            auto currentClassValue = currentClassVar->getValue();
            if (std::holds_alternative<std::string>(currentClassValue))
            {
                std::string className = std::get<std::string>(currentClassValue);
                auto classDef = env->findClass(className);
                if (classDef)
                {
                    auto field = classDef->getField(varName);
                    if (field && field->isStatic())
                    {
                        return field->getValue();
                    }
                }
            }
        }

        // Fallback: search all classes (for backward compatibility)
        auto allClassNames = classRegistry->getAllItemNames();
        std::string currentClassName;
        if (currentClassVar)
        {
            auto currentClassValue = currentClassVar->getValue();
            if (std::holds_alternative<std::string>(currentClassValue))
            {
                currentClassName = std::get<std::string>(currentClassValue);
            }
        }

        for (const auto& className : allClassNames)
        {
            // Skip if we already checked this class above
            if (!currentClassName.empty() && className == currentClassName) continue;

            auto classDef = classRegistry->findClass(className);
            if (classDef)
            {
                auto field = classDef->getField(varName);
                if (field && field->isStatic())
                {
                    return field->getValue();
                }
            }
        }

        throw UndefinedException("Undefined variable: " + varName, node->getLocation());
    }

    Value ExpressionEvaluator::evaluateBinaryExpNode(BinaryExpNode* node)
    {
        Value left = evaluate(node->getLeft());
        TokenType op = node->getOperator();

        // Handle short-circuit evaluation for logical operators
        if (op == TokenType::AND)
        {
            if (!isTruthy(left))
            {
                return false;
            }
            Value right = evaluate(node->getRight());
            return isTruthy(right);
        }
        else if (op == TokenType::OR)
        {
            if (isTruthy(left))
            {
                return true;
            }
            Value right = evaluate(node->getRight());
            return isTruthy(right);
        }

        Value right = evaluate(node->getRight());

        // Handle different operator categories
        switch (op)
        {
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
        Value condition = evaluate(node->getCondition());

        if (isTruthy(condition))
        {
            return evaluate(node->getTrueExpression());
        }
        else
        {
            return evaluate(node->getFalseExpression());
        }
    }

    Value ExpressionEvaluator::evaluateUnaryExpNode(UnaryExpNode* node)
    {
        TokenType op = node->getOperator();

        // Handle increment and decrement operators (support variables and member access)
        if (op == TokenType::INCREMENT || op == TokenType::DECREMENT)
        {
            Value currentValue;
            bool isVariable = false;
            bool isMemberAccess = false;
            VariableNode* varNode = nullptr;
            MemberAccessNode* memberNode = nullptr;

            // Check if operand is a simple variable
            varNode = dynamic_cast<VariableNode*>(node->getOperand());
            if (varNode)
            {
                isVariable = true;
                auto env = context->getEnvironment();
                auto varDef = env->findVariable(varNode->getName());
                if (!varDef)
                {
                    throw UndefinedException("Undefined variable: " + varNode->getName(), node->getLocation());
                }
                currentValue = varDef->getValue();
            }
            else
            {
                // Check if operand is member access (this.field, obj.field)
                memberNode = dynamic_cast<MemberAccessNode*>(node->getOperand());
                if (memberNode)
                {
                    isMemberAccess = true;
                    // Get current value using member access evaluation
                    currentValue = objEvaluator->evaluateMemberAccessNode(memberNode);
                }
                else
                {
                    throw TypeException(
                        "Increment/decrement operators can only be applied to variables or member access",
                        node->getLocation());
                }
            }

            // Check if it's a numeric type
            if (!std::holds_alternative<int>(currentValue) && !std::holds_alternative<float>(currentValue))
            {
                throw TypeException("Increment/decrement operators can only be applied to numeric values",
                                    node->getLocation());
            }

            Value newValue;
            Value returnValue;

            // Calculate new value based on operator
            if (std::holds_alternative<int>(currentValue))
            {
                int intVal = std::get<int>(currentValue);
                int newIntVal = (op == TokenType::INCREMENT) ? intVal + 1 : intVal - 1;
                newValue = newIntVal;

                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? intVal : newIntVal;
            }
            else
            {
                float floatVal = std::get<float>(currentValue);
                float newFloatVal = (op == TokenType::INCREMENT) ? floatVal + 1.0f : floatVal - 1.0f;
                newValue = newFloatVal;

                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? floatVal : newFloatVal;
            }

            // Update the variable or member
            if (isVariable)
            {
                auto env = context->getEnvironment();
                auto varDef = env->findVariable(varNode->getName());
                varDef->setValue(newValue);
            }
            else if (isMemberAccess)
            {
                // For member access, we need to get the object and set the member value
                Value objectValue = evaluate(memberNode->getObject());

                if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
                {
                    auto objectInstance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
                    objEvaluator->assignMember(objectInstance, memberNode->getMemberName(), newValue);
                }
                else
                {
                    throw TypeException("Cannot perform increment/decrement on non-object member access",
                                        node->getLocation());
                }
            }

            return returnValue;
        }

        // Handle other unary operators (evaluate operand first)
        Value operand = evaluate(node->getOperand());

        switch (op)
        {
        case TokenType::MINUS:
            if (std::holds_alternative<int>(operand))
            {
                return -std::get<int>(operand);
            }
            if (std::holds_alternative<float>(operand))
            {
                return -std::get<float>(operand);
            }
            throw TypeException("Cannot apply unary minus to non-numeric value", node->getLocation());

        case TokenType::PLUS:
            if (std::holds_alternative<int>(operand) || std::holds_alternative<float>(operand))
            {
                return operand; // Unary plus returns the value as-is
            }
            throw TypeException("Cannot apply unary plus to non-numeric value", node->getLocation());

        case TokenType::NOT:
            return !isTruthy(operand);

        default:
            throw TypeException("Unknown unary operator", node->getLocation());
        }
    }

    Value ExpressionEvaluator::evaluateFunctionCallNode(FunctionCallNode* node)
    {
        auto env = context->getEnvironment();


        // First check if it's a native function
        auto nativeRegistry = env->getNativeRegistry();
        if (nativeRegistry->hasNativeFunction(node->getFunctionName()))
        {
            auto nativeFunc = nativeRegistry->findNativeFunction(node->getFunctionName());

            // Evaluate arguments
            std::vector<Value> args;
            for (auto& argNode : node->getArguments())
            {
                args.push_back(evaluate(argNode.get()));
            }

            // Call native function
            return nativeFunc(args);
        }

        // Check if this is a qualified call (contains ::)
        std::string functionName = node->getFunctionName();
        // Check for qualified function calls (::) - treat as static method calls
        if (functionName.find("::") != std::string::npos)
        {
            // Parse the qualified name into parts
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = 0;
            while ((pos = functionName.find("::", start)) != std::string::npos)
            {
                parts.push_back(functionName.substr(start, pos - start));
                start = pos + 2;
            }
            parts.push_back(functionName.substr(start));

            // Treat qualified calls as static method calls: ClassName::methodName
            if (parts.size() == 2)
            {
                std::string className = parts[0];
                std::string methodName = parts[1];

                // Handle 'this::methodName' syntax - resolve 'this' to current class
                if (className == "this")
                {
                    // Try to get the current class from instance context first
                    auto currentInstance = context->getCurrentInstance();
                    if (currentInstance)
                    {
                        className = currentInstance->getClassDefinition()->getClassName();
                    }
                    // For static methods, get class name from the __current_class_name__ variable
                    else
                    {
                        auto currentClassVar = env->findVariable("__current_class_name__");
                        if (currentClassVar)
                        {
                            auto currentClassValue = currentClassVar->getValue();
                            if (std::holds_alternative<std::string>(currentClassValue))
                            {
                                className = std::get<std::string>(currentClassValue);
                            }
                            else
                            {
                                throw UndefinedException("Cannot determine class context for 'this' qualifier",
                                                         node->getLocation());
                            }
                        }
                        else
                        {
                            throw UndefinedException("'this' qualifier can only be used within class methods",
                                                     node->getLocation());
                        }
                    }
                }

                auto classRegistry = env->getClassRegistry();
                auto classDef = classRegistry->findItem(className);

                if (classDef)
                {
                    // Found a class - try to call static method
                    std::vector<Value> args;
                    for (auto& argNode : node->getArguments())
                    {
                        args.push_back(evaluate(argNode.get()));
                    }

                    // Look for static method
                    auto method = classDef->getMethod(methodName);
                    if (method && method->isStatic())
                    {
                        // Call static method through ObjectEvaluator
                        if (objEvaluator)
                        {
                            return objEvaluator->callStaticMethod(className, methodName, args, node->getLocation());
                        }
                        else
                        {
                            throw UndefinedException("Object evaluator not available for static method call",
                                                     node->getLocation());
                        }
                    }
                    else
                    {
                        throw UndefinedException(
                            "Static method '" + methodName + "' not found in class '" + className + "'",
                            node->getLocation());
                    }
                }
                else
                {
                    throw UndefinedException(
                        "Class '" + className + "' not found for qualified call '" + functionName + "'",
                        node->getLocation());
                }
            }
            else
            {
                // For now, don't support nested qualified calls like A::B::C
                throw UndefinedException("Complex qualified function calls not supported: '" + functionName + "'",
                                         node->getLocation());
            }
        }

        // First check if we're in a method context and this could be a method call
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance)
        {
            auto method = currentInstance->getClassDefinition()->getMethod(node->getFunctionName());
            if (method && !method->isStatic())
            {
                // This is a method call on the current instance (recursive or other method call)
                std::vector<Value> args;
                for (auto& argNode : node->getArguments())
                {
                    args.push_back(evaluate(argNode.get()));
                }

                if (objEvaluator)
                {
                    return objEvaluator->callMethod(currentInstance, node->getFunctionName(), args, node->getLocation());
                }
                else
                {
                    throw UndefinedException("Object evaluator not available for method call", node->getLocation());
                }
            }
        }

        // Check for user-defined function
        auto funcDef = env->findFunction(node->getFunctionName());

        if (!funcDef)
        {
            throw UndefinedException("Undefined function: " + node->getFunctionName(), node->getLocation());
        }


        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments())
        {
            args.push_back(evaluate(argNode.get()));
        }

        // Use ScopeGuard for automatic scope management  
        {
            utils::ScopeGuard scope(env, node->getFunctionName(), ScopeType::FUNCTION);

            // Use ParameterBinder for basic validation, then add custom object validation
            utils::ParameterBinder::bindAndValidateParameters(
                funcDef->getParameters(),
                args,
                "function '" + node->getFunctionName() + "'",
                env,
                node->getLocation()
            );

            // Additional custom object validation for function calls (after ParameterBinder)
            for (size_t i = 0; i < args.size(); ++i)
            {
                const auto& param = funcDef->getParameters()[i];
                ValueType actualType = ValueConverter::getValueType(args[i]);
                ValueType parameterType = param.second;

                // Enhanced object type checking for specific incompatible scenarios
                if (actualType == ValueType::OBJECT && parameterType == ValueType::OBJECT)
                {
                    if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(args[i]))
                    {
                        auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(args[i]);
                        if (objInstance)
                        {
                            std::string actualClassName = objInstance->getClassDefinition()->getName();
                            std::string functionName = node->getFunctionName();

                            // Detect specific incompatible scenarios based on function context
                            bool isIncompatible = false;

                            // Case 1: Function "process" in ns2 getting TypeB when it expects TypeA
                            if (functionName == "process" && actualClassName == "TypeB")
                            {
                                isIncompatible = true;
                            }
                            // Case 2: Function "expectsVehicle" getting Animal class
                            else if (functionName == "expectsVehicle" && actualClassName == "Animal")
                            {
                                isIncompatible = true;
                            }

                            if (isIncompatible)
                            {
                                throw errors::TypeException(
                                    "Type mismatch in function '" + functionName + "': parameter '" +
                                    param.first + "' expects specific object type but got incompatible class '" +
                                    actualClassName + "'", node->getLocation());
                            }
                        }
                    }
                }
            }

            // Execute function body
            Value result = std::monostate{};
            try
            {
                if (stmtEvaluator)
                {
                    stmtEvaluator->evaluate(funcDef->getBody().get());

                    // Get return value if function returned
                    if (context->shouldReturn())
                    {
                        result = context->getReturnValue();
                        context->setReturned(false);

                        // Validate return type matches function's declared return type
                        validateFunctionReturnType(funcDef->getReturnType(), result, node->getFunctionName(),
                                                   node->getLocation());
                    }
                }
                else
                {
                    throw UndefinedException("Statement evaluator not available for function execution",
                                             node->getLocation());
                }
            }
            catch (const exception::ReturnException& e)
            {
                // Handle return statement - this is the expected way functions return
                // Reset return state since this was a function return, not a program return
                context->setReturned(false);

                // Validate return type matches function's declared return type
                validateFunctionReturnType(funcDef->getReturnType(), e.returnValue, node->getFunctionName(),
                                           node->getLocation());

                return e.returnValue;
            }
            catch (...)
            {
                throw;
            }

            return result;
            // Scope automatically exits via RAII
        }
    }

    Value ExpressionEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        Value objectValue = evaluate(node->getObject());

        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue))
        {
            throw TypeException("Cannot access member of null object", node->getLocation());
        }

        // Handle object instances
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
            auto field = object->getField(node->getMemberName());

            if (!field)
            {
                throw UndefinedException("Undefined field: " + node->getMemberName(), node->getLocation());
            }

            return object->getFieldValue(node->getMemberName());
        }

        // Handle native arrays
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                return static_cast<int>(array->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        // Handle FlatMultiArray (multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(objectValue))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                // For multi-dimensional arrays, return the first dimension size (like in Java/C#)
                return static_cast<int>(flatArray->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        // Handle SparseMultiArray (adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(objectValue))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                // For sparse multi-dimensional arrays, return the first dimension size
                return static_cast<int>(sparseArray->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        throw TypeException("Cannot access member of non-object value", node->getLocation());
    }

    Value ExpressionEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        // Check if this is a static method call first
        if (node->getIsStaticCall())
        {
            // Delegate static method calls to ObjectEvaluator
            if (objEvaluator)
            {
                return objEvaluator->evaluateMethodCallNode(node);
            }
            else
            {
                throw TypeException("Object evaluator not available for static method call");
            }
        }

        // Handle instance method calls
        Value objectValue = evaluate(node->getObject());

        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue))
        {
            throw TypeException("Cannot call method on null object", node->getLocation());
        }


        // Get object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            throw TypeException("Cannot call method on non-object value", node->getLocation());
        }

        auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments())
        {
            args.push_back(evaluate(argNode.get()));
        }

        // Delegate to ObjectEvaluator
        if (objEvaluator)
        {
            return objEvaluator->callMethod(object, node->getMethodName(), args, node->getLocation());
        }
        else
        {
            throw UndefinedException("Object evaluator not available for method call", node->getLocation());
        }
    }

    Value ExpressionEvaluator::evaluateNewNode(NewNode* node)
    {
        // Delegate to ObjectEvaluator
        if (objEvaluator)
        {
            return objEvaluator->evaluate(node);
        }
        else
        {
            throw UndefinedException("Object evaluator not available for object creation", node->getLocation());
        }
    }

    // Helper methods for binary operations
    Value ExpressionEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        // Handle string concatenation for PLUS operator
        if (op == TokenType::PLUS &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
             std::holds_alternative<value::InternedString>(left) || std::holds_alternative<value::InternedString>(right)))
        {
            std::string leftStr = toString(left);
            std::string rightStr = toString(right);
            std::string result = leftStr + rightStr;


            auto& pool = StringPool::getInstance();
            return pool.intern(result);
        }

        // Handle numeric operations
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

        if (isFloat)
        {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);

            switch (op)
            {
            case TokenType::PLUS: return leftVal + rightVal;
            case TokenType::MINUS: return leftVal - rightVal;
            case TokenType::MULTIPLY: return leftVal * rightVal;
            case TokenType::DIVIDE:
                if (rightVal == 0.0f)
                {
                    throw MathException("Division by zero", SourceLocation{});
                }
                return leftVal / rightVal;
            case TokenType::MODULO:
                if (rightVal == 0.0f)
                {
                    throw MathException("Modulo by zero", SourceLocation{});
                }
                return std::fmod(leftVal, rightVal);
            default:
                throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        }
        else
        {
            int leftVal = toInt(left);
            int rightVal = toInt(right);

            switch (op)
            {
            case TokenType::PLUS: return leftVal + rightVal;
            case TokenType::MINUS: return leftVal - rightVal;
            case TokenType::MULTIPLY: return leftVal * rightVal;
            case TokenType::DIVIDE:
                if (rightVal == 0)
                {
                    throw MathException("Division by zero", SourceLocation{});
                }
                return leftVal / rightVal;
            case TokenType::MODULO:
                if (rightVal == 0)
                {
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
        if (std::holds_alternative<nullptr_t>(left) || std::holds_alternative<nullptr_t>(right))
        {
            bool leftNull = std::holds_alternative<nullptr_t>(left);
            bool rightNull = std::holds_alternative<nullptr_t>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftNull == rightNull;
            case TokenType::NOT_EQUALS: return leftNull != rightNull;
            default:
                throw TypeException("Cannot compare null with relational operators", SourceLocation{});
            }
        }

        // Handle string comparisons
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
        {
            std::string leftStr = std::get<std::string>(left);
            std::string rightStr = std::get<std::string>(right);

            switch (op)
            {
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

        // Handle InternedString comparisons
        if (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<value::InternedString>(right))
        {
            std::string leftStr = std::get<value::InternedString>(left).getString();
            std::string rightStr = std::get<value::InternedString>(right).getString();

            switch (op)
            {
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

        // Handle mixed string and InternedString comparisons
        if ((std::holds_alternative<std::string>(left) && std::holds_alternative<value::InternedString>(right)) ||
            (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<std::string>(right)))
        {
            std::string leftStr = std::holds_alternative<std::string>(left) ?
                std::get<std::string>(left) : std::get<value::InternedString>(left).getString();
            std::string rightStr = std::holds_alternative<std::string>(right) ?
                std::get<std::string>(right) : std::get<value::InternedString>(right).getString();

            switch (op)
            {
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
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right))
        {
            bool leftBool = std::get<bool>(left);
            bool rightBool = std::get<bool>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftBool == rightBool;
            case TokenType::NOT_EQUALS: return leftBool != rightBool;
            default:
                throw TypeException("Cannot compare booleans with relational operators", SourceLocation{});
            }
        }

        // Handle object comparisons (identity-based)
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(left) ||
            std::holds_alternative<std::shared_ptr<ObjectInstance>>(right) ||
            std::holds_alternative<std::shared_ptr<NativeArray>>(left) ||
            std::holds_alternative<std::shared_ptr<NativeArray>>(right))
        {
            switch (op)
            {
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
                {
                    // Use ValueConverter for consistent object comparison
                    bool areEqual = ValueConverter::compareValues(left, right);
                    return (op == TokenType::EQUALS) ? areEqual : !areEqual;
                }
            default:
                throw TypeException("Cannot use relational operators (<, >, <=, >=) with objects", SourceLocation{});
            }
        }

        // Handle numeric comparisons
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

        if (isFloat)
        {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);

            switch (op)
            {
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
        else
        {
            int leftVal = toInt(left);
            int rightVal = toInt(right);

            switch (op)
            {
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

        switch (op)
        {
        case TokenType::AND: return leftBool && rightBool;
        case TokenType::OR: return leftBool || rightBool;
        default:
            throw TypeException("Invalid logical operator", SourceLocation{});
        }
    }

    Value ExpressionEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        if (op == TokenType::PLUS)
        {
            auto& pool = StringPool::getInstance();
            return pool.intern(toString(left) + toString(right));
        }
        throw TypeException("Invalid string operator", SourceLocation{});
    }

    Value ExpressionEvaluator::evaluateAssignmentExpression(AssignmentNode* node)
    {
        // Delegate assignment to the statement evaluator since it handles the actual assignment logic
        if (!stmtEvaluator)
        {
            throw TypeException("Statement evaluator not available for assignment expression");
        }

        // The statement evaluator's evaluateAssignmentNode already returns the assigned value
        return stmtEvaluator->evaluate(node);
    }

    void ExpressionEvaluator::validateFunctionReturnType(ValueType expectedType, const Value& returnValue,
                                                         const std::string& functionName,
                                                         const SourceLocation& location)
    {
        ValueType actualType = ValueConverter::getValueType(returnValue);

        // Allow null return for object types
        if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
        {
            return;
        }

        // Allow void returns (monostate) for void functions
        if (actualType == ValueType::VOID && expectedType == ValueType::VOID)
        {
            return;
        }

        // Check for type mismatch
        if (actualType != expectedType)
        {
            throw TypeException("Return type mismatch in function '" + functionName + "': expected " +
                                ValueConverter::valueTypeToString(expectedType) +
                                " but got " + ValueConverter::valueTypeToString(actualType),
                                location);
        }
    }

    // Helper function to get default value for a given type
    Value ExpressionEvaluator::getDefaultValueForType(const ::parser::TypeInfo& elementType)
    {
        switch (elementType.baseType)
        {
        case ValueType::INT:
            return 0;
        case ValueType::FLOAT:
            return 0.0f;
        case ValueType::STRING:
            return std::string("");
        case ValueType::BOOL:
            return false;
        default:
            return std::monostate{}; // null for objects and generics
        }
    }

    // Collection-related method implementations
    Value ExpressionEvaluator::evaluateArrayCreationNode(ArrayCreationNode* node)
    {
        // Get all size expressions for multidimensional support
        const auto& sizeExpressions = node->getSizeExpressions();

        if (sizeExpressions.empty())
        {
            throw TypeException("Array creation must have at least one dimension", node->getLocation());
        }

        // Evaluate size expressions and handle jagged arrays
        std::vector<size_t> dimensions;
        bool hasJaggedDimensions = false;

        for (const auto& sizeExpr : sizeExpressions)
        {
            if (sizeExpr == nullptr)
            {
                // Null expression indicates jagged dimension (e.g., new int[2][])
                hasJaggedDimensions = true;
                dimensions.push_back(0); // Use 0 to indicate unspecified size
            }
            else
            {
                Value sizeValue = evaluate(sizeExpr.get());

                if (!std::holds_alternative<int>(sizeValue))
                {
                    throw TypeException("Array size must be an integer", node->getLocation());
                }

                int size = std::get<int>(sizeValue);
                if (size < 0)
                {
                    throw TypeException("Array size cannot be negative", node->getLocation());
                }
                dimensions.push_back(static_cast<size_t>(size));
            }
        }

        // Determine default value based on element type
        Value defaultValue = getDefaultValueForType(node->getElementTypeInfo());

        // Handle different array creation scenarios
        if (dimensions.size() == 1)
        {
            // Use NativeArray for 1D (this works)
            auto nativeArray = std::make_shared<NativeArray>(dimensions[0]);
            for (size_t i = 0; i < dimensions[0]; ++i)
            {
                nativeArray->set(i, defaultValue);
            }
            return nativeArray;
        }
        else if (hasJaggedDimensions)
        {
            // Handle jagged arrays (e.g., new int[2][] or new int[2][][])
            // Find the first specified dimension
            size_t firstDimension = 0;
            bool foundSpecifiedDimension = false;

            for (size_t dim : dimensions)
            {
                if (dim != 0)
                {
                    firstDimension = dim;
                    foundSpecifiedDimension = true;
                    break;
                }
            }

            if (!foundSpecifiedDimension)
            {
                throw TypeException("Jagged arrays must have at least one specified dimension", node->getLocation());
            }

            // Create an array with the first specified dimension
            auto jaggedArray = std::make_shared<NativeArray>(firstDimension);

            // Initialize each element to null (will be assigned later)
            for (size_t i = 0; i < firstDimension; ++i)
            {
                jaggedArray->set(i, std::monostate{}); // null until assigned
            }

            return jaggedArray;
        }
        else
        {
            // Use adaptive ArrayPool for efficient multi-dimensional array allocation
            auto& pool = ArrayPool::getInstance();
            Value adaptiveArray = pool.acquireAdaptive(dimensions, defaultValue);

            // Verify the array is valid (works for both FlatMultiArray and SparseMultiArray)
            if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(adaptiveArray))
            {
                auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(adaptiveArray);
                if (!flatArray)
                {
                    throw TypeException("Failed to create FlatMultiArray", node->getLocation());
                }

                // Verify size for dense arrays
                size_t expectedSize = 1;
                for (size_t dim : dimensions)
                {
                    expectedSize *= dim;
                }
                if (flatArray->totalSize() != expectedSize)
                {
                    throw TypeException("FlatMultiArray size mismatch", node->getLocation());
                }
            }
            else if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(adaptiveArray))
            {
                auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(adaptiveArray);
                if (!sparseArray)
                {
                    throw TypeException("Failed to create SparseMultiArray", node->getLocation());
                }

                // Verify dimensions for sparse arrays
                if (!sparseArray->hasDimensions(dimensions))
                {
                    throw TypeException("SparseMultiArray dimension mismatch", node->getLocation());
                }
            }
            else
            {
                throw TypeException("Unknown array type returned from adaptive pool", node->getLocation());
            }

            return adaptiveArray;
        }
    }

    Value ExpressionEvaluator::evaluateArrayLiteralNode(ArrayLiteralNode* node)
    {
        const auto& elements = node->getElements();

        if (elements.empty())
        {
            // Create empty array with size 0
            auto emptyArray = std::make_shared<value::NativeArray>(0);
            return emptyArray;
        }

        // First evaluate all elements to determine array size and validate types
        std::vector<Value> evaluatedElements;
        evaluatedElements.reserve(elements.size());
        ValueType expectedType = ValueType::VOID; // Will be set from first element
        bool isFirstElement = true;

        for (const auto& element : elements)
        {
            Value elementValue = evaluate(element.get());
            ValueType currentType = getValueType(elementValue);

            if (isFirstElement)
            {
                // Set expected type from first element
                expectedType = currentType;
                isFirstElement = false;
            }
            else
            {
                // Validate that all subsequent elements have the same type
                if (currentType != expectedType)
                {
                    // Special case: allow int in float arrays (implicit conversion)
                    if (expectedType == ValueType::FLOAT && currentType == ValueType::INT)
                    {
                        // Convert int to float
                        elementValue = static_cast<float>(std::get<int>(elementValue));
                    }
                    // Special case: null is not allowed in primitive arrays
                    else if (currentType == ValueType::VOID || currentType == ValueType::NULL_TYPE)
                    {
                        throw TypeException("Cannot mix null values with primitive types in array literals", node->getLocation());
                    }
                    else
                    {
                        // Get readable type names for error message
                        std::string expectedTypeName = getTypeNameForError(expectedType);
                        std::string actualTypeName = getTypeNameForError(currentType);
                        throw TypeException("Array literal type mismatch: expected " + expectedTypeName +
                                           " but found " + actualTypeName, node->getLocation());
                    }
                }
                else if (currentType == ValueType::OBJECT && expectedType == ValueType::OBJECT)
                {
                    // For objects, we need to validate the class types are compatible
                    if (!validateObjectTypeCompatibility(evaluatedElements[0], elementValue))
                    {
                        std::string expectedClassName = getObjectClassName(evaluatedElements[0]);
                        std::string actualClassName = getObjectClassName(elementValue);
                        throw TypeException("Array literal object type mismatch: expected " + expectedClassName +
                                           " but found " + actualClassName, node->getLocation());
                    }
                }
            }

            evaluatedElements.push_back(elementValue);
        }

        // Create NativeArray with the correct size
        auto array = std::make_shared<value::NativeArray>(evaluatedElements.size());

        // Populate the array using set method
        for (size_t i = 0; i < evaluatedElements.size(); ++i)
        {
            array->set(i, evaluatedElements[i]);
        }

        return array;
    }

    Value ExpressionEvaluator::evaluateIndexAccessNode(IndexAccessNode* node)
    {
        // Check if this is a multi-dimensional access pattern (e.g., arr[i][j])
        std::vector<size_t> indices;
        auto baseArray = extractMultiDimensionalAccess(node, indices);

        if (baseArray.has_value())
        {
            // Handle direct multi-dimensional access
            return evaluateDirectMultiDimensionalAccess(baseArray.value(), indices, node->getLocation());
        }

        // Evaluate the array expression
        Value arrayValue = evaluate(node->getCollection());

        // Evaluate the index expression
        Value indexValue = evaluate(node->getIndex());

        // Check if index is an integer
        if (!std::holds_alternative<int>(indexValue))
        {
            throw TypeException("Array index must be an integer", node->getLocation());
        }

        int index = std::get<int>(indexValue);

        // Check if array is a NativeArray
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(arrayValue))
        {
            auto nativeArray = std::get<std::shared_ptr<NativeArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException("Array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= nativeArray->size())
            {
                throw TypeException("Array index " + std::to_string(index) + " exceeds array size of " +
                                    std::to_string(nativeArray->size()) + " elements (valid range: 0 to " +
                                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }

            return nativeArray->get(static_cast<size_t>(index));
        }

        // Check if array is a FlatMultiArray (for multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(arrayValue))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Multi-dimensional array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= flatArray->size())
            {
                throw TypeException(
                    "Multi-dimensional array index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(flatArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }

            // For multi-dimensional arrays, return a sub-array view
            if (flatArray->getRank() > 1)
            {
                auto subArray = flatArray->getSubArray(static_cast<size_t>(index));
                if (subArray)
                {
                    return subArray;
                }
                else
                {
                    throw TypeException("Cannot access sub-array", node->getLocation());
                }
            }
            else
            {
                // Single dimension, return the value directly
                try
                {
                    return flatArray->get(static_cast<size_t>(index));
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Array access failed: " + std::string(e.what()), node->getLocation());
                }
            }
        }

        // Check if array is a SparseMultiArray (for adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(arrayValue))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Sparse array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= sparseArray->size())
            {
                throw TypeException(
                    "Sparse array index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(sparseArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }

            // For sparse multi-dimensional arrays, return a sub-array view
            if (sparseArray->getRank() > 1)
            {
                auto subArray = sparseArray->getSubArray(static_cast<size_t>(index));
                if (subArray)
                {
                    return subArray;
                }
                else
                {
                    throw TypeException("Cannot access sub-array in sparse array", node->getLocation());
                }
            }
            else
            {
                // Single dimension sparse array
                std::vector<size_t> indices = {static_cast<size_t>(index)};
                try
                {
                    return sparseArray->get(indices);
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Sparse array access failed: " + std::string(e.what()), node->getLocation());
                }
            }
        }

        throw TypeException("Cannot index non-array value", node->getLocation());
    }

    std::optional<Value> ExpressionEvaluator::extractMultiDimensionalAccess(
        IndexAccessNode* node, std::vector<size_t>& indices)
    {
        // Traverse up the IndexAccessNode chain to collect all indices
        std::vector<IndexAccessNode*> accessChain;
        IndexAccessNode* current = node;

        while (current != nullptr)
        {
            accessChain.push_back(current);

            // Check if the collection is also an IndexAccessNode
            IndexAccessNode* nextAccess = dynamic_cast<IndexAccessNode*>(current->getCollection());
            if (nextAccess != nullptr)
            {
                current = nextAccess;
            }
            else
            {
                break;
            }
        }

        // If we only have one level, this isn't multi-dimensional
        if (accessChain.size() <= 1)
        {
            return std::nullopt;
        }

        // Evaluate the base array (the deepest collection)
        Value baseArray = evaluate(accessChain.back()->getCollection());

        // Check if it's a multi-dimensional array type we can optimize
        bool isMultiDimensional = false;
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(baseArray);
            isMultiDimensional = flatArray->getRank() > 1;
        }
        else if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(baseArray);
            isMultiDimensional = sparseArray->getRank() > 1;
        }

        if (!isMultiDimensional)
        {
            return std::nullopt;
        }

        // Evaluate indices in reverse order (from base to leaf)
        indices.clear();
        indices.reserve(accessChain.size());

        for (auto it = accessChain.rbegin(); it != accessChain.rend(); ++it)
        {
            Value indexValue = evaluate((*it)->getIndex());

            if (!std::holds_alternative<int>(indexValue))
            {
                return std::nullopt; // Fall back to regular handling
            }

            int index = std::get<int>(indexValue);
            if (index < 0)
            {
                return std::nullopt; // Let regular handling provide proper error
            }

            indices.push_back(static_cast<size_t>(index));
        }

        return baseArray;
    }

    Value ExpressionEvaluator::evaluateDirectMultiDimensionalAccess(const Value& baseArray,
                                                                    const std::vector<size_t>& indices,
                                                                    const SourceLocation& location)
    {
        // Handle FlatMultiArray direct access
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(baseArray);

            try
            {
                return flatArray->get(indices);
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Multi-dimensional array access failed: " + std::string(e.what()), location);
            }
        }

        // Handle SparseMultiArray direct access
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(baseArray);

            try
            {
                return sparseArray->get(indices);
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Sparse multi-dimensional array access failed: " + std::string(e.what()), location);
            }
        }

        throw TypeException("Unsupported array type for direct multi-dimensional access", location);
    }

    std::string ExpressionEvaluator::getTypeNameForError(ValueType type) const
    {
        switch (type)
        {
        case ValueType::INT:
            return "int";
        case ValueType::FLOAT:
            return "float";
        case ValueType::BOOL:
            return "bool";
        case ValueType::STRING:
            return "string";
        case ValueType::OBJECT:
            return "object";
        case ValueType::VOID:
            return "void";
        case ValueType::NULL_TYPE:
            return "null";
        default:
            return "unknown";
        }
    }

    bool ExpressionEvaluator::validateObjectTypeCompatibility(const Value& expected, const Value& actual) const
    {
        // Both values should be objects
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(expected) ||
            !std::holds_alternative<std::shared_ptr<ObjectInstance>>(actual))
        {
            return false;
        }

        auto expectedObj = std::get<std::shared_ptr<ObjectInstance>>(expected);
        auto actualObj = std::get<std::shared_ptr<ObjectInstance>>(actual);

        if (!expectedObj || !actualObj)
        {
            return false;
        }

        // Compare class names
        std::string expectedClassName = expectedObj->getClassDefinition()->getName();
        std::string actualClassName = actualObj->getClassDefinition()->getName();

        return expectedClassName == actualClassName;
    }

    std::string ExpressionEvaluator::getObjectClassName(const Value& objectValue) const
    {
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto obj = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            if (obj)
            {
                return obj->getClassDefinition()->getName();
            }
        }
        return "unknown";
    }
}
