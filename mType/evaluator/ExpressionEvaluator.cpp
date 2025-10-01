#include "ExpressionEvaluator.hpp"
#include "expressions/LiteralEvaluator.hpp"
#include "expressions/BinaryOperationEvaluator.hpp"
#include "expressions/CallHandler.hpp"
#include "expressions/ArrayHandler.hpp"
#include "expressions/UnaryOperationHandler.hpp"
#include "expressions/AccessHandler.hpp"
#include "utils/ParameterBinder.hpp"
#include "utils/ObjectHelper.hpp"
#include "validation/TypeValidator.hpp"
#include "../value/StringPool.hpp"
#include "../value/LambdaValue.hpp"
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "utils/ScopeGuard.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/MathException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/RuntimeException.hpp"
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
#include "utils/ValueConverter.hpp"
#include <cmath>

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes;
    using namespace runtimeTypes::global;
    using namespace runtimeTypes::klass;
    using namespace value;

    ExpressionEvaluator::ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx)
          , literalEvaluator(std::make_unique<expressions::LiteralEvaluator>(ctx))
          , binaryOpEvaluator(std::make_unique<expressions::BinaryOperationEvaluator>(ctx))
          , callHandler(std::make_unique<expressions::CallHandler>(ctx))
          , arrayHandler(std::make_unique<expressions::ArrayHandler>(ctx))
          , unaryOpHandler(std::make_unique<expressions::UnaryOperationHandler>(ctx))
          , accessHandler(std::make_unique<expressions::AccessHandler>(ctx))
          , stmtEvaluator(nullptr)
          , objEvaluator(nullptr)
    {
        // Set back-references
        binaryOpEvaluator->setExpressionEvaluator(this);
        callHandler->setExpressionEvaluator(this);
        arrayHandler->setExpressionEvaluator(this);
        unaryOpHandler->setExpressionEvaluator(this);
        accessHandler->setExpressionEvaluator(this);
    }

    // Destructor must be defined in .cpp where complete types are available
    ExpressionEvaluator::~ExpressionEvaluator() = default;

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
        if (auto lambdaNode = dynamic_cast<LambdaNode*>(node))
        {
            return evaluateLambdaNode(lambdaNode);
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
        if (auto lambdaInterfaceNode = dynamic_cast<LambdaInterfaceInvocationNode*>(node))
        {
            return evaluateLambdaInterfaceInvocationNode(lambdaInterfaceNode);
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
        callHandler->setStatementEvaluator(evaluator);
        accessHandler->setStatementEvaluator(evaluator);
    }

    void ExpressionEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
        callHandler->setObjectEvaluator(evaluator);
        unaryOpHandler->setObjectEvaluator(evaluator);
        accessHandler->setObjectEvaluator(evaluator);
    }

    bool ExpressionEvaluator::isExpressionNode(ASTNode* node) const
    {
        return dynamic_cast<IntegerNode*>(node) ||
            dynamic_cast<FloatNode*>(node) ||
            dynamic_cast<StringNode*>(node) ||
            dynamic_cast<BoolNode*>(node) ||
            dynamic_cast<NullNode*>(node) ||
            dynamic_cast<LambdaNode*>(node) ||
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
        return literalEvaluator->evaluateInteger(node);
    }

    Value ExpressionEvaluator::evaluateFloatNode(FloatNode* node)
    {
        return literalEvaluator->evaluateFloat(node);
    }

    Value ExpressionEvaluator::evaluateStringNode(StringNode* node)
    {
        return literalEvaluator->evaluateString(node);
    }

    Value ExpressionEvaluator::evaluateBoolNode(BoolNode* node)
    {
        return literalEvaluator->evaluateBool(node);
    }

    Value ExpressionEvaluator::evaluateNullNode(NullNode* node)
    {
        return literalEvaluator->evaluateNull(node);
    }

    Value ExpressionEvaluator::evaluateVariableNode(VariableNode* node)
    {
        return accessHandler->evaluateVariableAccess(node);
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
        return unaryOpHandler->evaluateUnaryOperation(node);
    }

    Value ExpressionEvaluator::evaluateFunctionCallNode(FunctionCallNode* node)
    {
        return callHandler->evaluateFunctionCall(node);
    }

    Value ExpressionEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        return accessHandler->evaluateMemberAccess(node);
    }

    Value ExpressionEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        return callHandler->evaluateMethodCall(node);
    }

    Value ExpressionEvaluator::evaluateNewNode(NewNode* node)
    {
        return accessHandler->evaluateObjectCreation(node);
    }

    // Delegate binary operations to BinaryOperationEvaluator
    Value ExpressionEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateArithmetic(left, right, op);
    }

    Value ExpressionEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateComparison(left, right, op);
    }

    Value ExpressionEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateLogical(left, right, op);
    }

    Value ExpressionEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateStringOperation(left, right, op);
    }

    Value ExpressionEvaluator::evaluateAssignmentExpression(AssignmentNode* node)
    {
        return accessHandler->evaluateAssignment(node);
    }


    // Helper function to get default value for a given type
    Value ExpressionEvaluator::getDefaultValueForType(const ::parser::TypeInfo& elementType)
    {
        return arrayHandler->getDefaultValueForType(elementType);
    }

    // Collection-related method implementations
    Value ExpressionEvaluator::evaluateArrayCreationNode(ArrayCreationNode* node)
    {
        return arrayHandler->evaluateArrayCreation(node);
    }

    Value ExpressionEvaluator::evaluateArrayLiteralNode(ArrayLiteralNode* node)
    {
        return arrayHandler->evaluateArrayLiteral(node);
    }

    Value ExpressionEvaluator::evaluateIndexAccessNode(IndexAccessNode* node)
    {
        return arrayHandler->evaluateIndexAccess(node);
    }

    std::optional<Value> ExpressionEvaluator::extractMultiDimensionalAccess(
        IndexAccessNode* node, std::vector<size_t>& indices)
    {
        return arrayHandler->extractMultiDimensionalAccess(node, indices);
    }

    Value ExpressionEvaluator::evaluateDirectMultiDimensionalAccess(const Value& baseArray,
                                                                    const std::vector<size_t>& indices,
                                                                    const SourceLocation& location)
    {
        return arrayHandler->evaluateDirectMultiDimensionalAccess(baseArray, indices, location);
    }

    Value ExpressionEvaluator::evaluateLambdaNode(LambdaNode* node)
    {
        // Create a LambdaValue with the current evaluation context
        // The LambdaValue will capture the current environment for closure support
        auto lambdaValue = std::make_shared<value::LambdaValue>(node, context);

        return lambdaValue;
    }

    Value ExpressionEvaluator::evaluateLambdaInterfaceInvocationNode(LambdaInterfaceInvocationNode* node)
    {
        return callHandler->evaluateLambdaInterfaceInvocation(node);
    }
}
