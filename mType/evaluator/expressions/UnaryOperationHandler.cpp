#include "UnaryOperationHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../ObjectEvaluator.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../token/TokenType.hpp"

using namespace errors;
using namespace token;
using namespace runtimeTypes::klass;
using namespace ast::nodes::classes;

namespace evaluator {
namespace expressions {

    Value UnaryOperationHandler::evaluateUnaryOperation(UnaryExpNode* node)
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
                Value objectValue = exprEvaluator->evaluate(memberNode->getObject());

                if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
                {
                    auto objectInstance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
                    objEvaluator->assignMember(objectInstance, memberNode->getMemberName(), newValue, node->getLocation());
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
        Value operand = exprEvaluator->evaluate(node->getOperand());

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
            return !exprEvaluator->isTruthy(operand);

        default:
            throw TypeException("Unknown unary operator", node->getLocation());
        }
    }

} // namespace expressions
} // namespace evaluator
