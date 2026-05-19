#include "TypeInferenceEngine.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../token/TokenType.hpp"

namespace vm::compiler::types
{
    // Operator overloading applies when at least one operand is a Box-type
    // object (Int/Float/Bool/String) and the *other* operand is a "simple"
    // expression: literal, variable, member access, index access, new, or a
    // binary op that itself yields a Box type. Complex receivers (e.g. method
    // calls) bypass overloading to keep inference linear-time.
    bool TypeInferenceEngine::tryBinaryOperatorOverload(
        ast::BinaryOpNode* binOp,
        value::ValueType leftType,
        value::ValueType rightType,
        std::string& outLeftClassName) const
    {
        using namespace ast::nodes::expressions;

        outLeftClassName.clear();
        bool willUseOperatorOverloading = false;
        bool usePrimitiveStringConcat = false;
        auto op = binOp->getOperator();

        if (leftType == value::ValueType::OBJECT)
        {
            outLeftClassName = inferExpressionClassName(binOp->getLeft());
            willUseOperatorOverloading = (outLeftClassName == "Int" || outLeftClassName == "Float" ||
                                          outLeftClassName == "Bool" || outLeftClassName == "String");
        }
        else if (rightType == value::ValueType::OBJECT)
        {
            // primitive-string + boxed: prefer primitive concat (no String class needed)
            if (leftType == value::ValueType::STRING && op == token::TokenType::PLUS)
            {
                usePrimitiveStringConcat = true;
            }
            else if (dynamic_cast<IntegerNode*>(binOp->getLeft()))
            {
                outLeftClassName = "Int";
                willUseOperatorOverloading = true;
            }
            else if (dynamic_cast<FloatNode*>(binOp->getLeft()))
            {
                outLeftClassName = "Float";
                willUseOperatorOverloading = true;
            }
            else if (dynamic_cast<BoolNode*>(binOp->getLeft()))
            {
                outLeftClassName = "Bool";
                willUseOperatorOverloading = true;
            }
        }

        bool rightIsSimple = false;
        if (rightType == value::ValueType::OBJECT)
        {
            if (dynamic_cast<ast::VariableNode*>(binOp->getRight()) ||
                dynamic_cast<ast::NewNode*>(binOp->getRight()) ||
                dynamic_cast<ast::MemberAccessNode*>(binOp->getRight()) ||
                dynamic_cast<IndexAccessNode*>(binOp->getRight()) ||
                dynamic_cast<ast::BinaryOpNode*>(binOp->getRight()))
            {
                rightIsSimple = true;
            }
        }
        else
        {
            if (dynamic_cast<IntegerNode*>(binOp->getRight()) ||
                dynamic_cast<FloatNode*>(binOp->getRight()) ||
                dynamic_cast<BoolNode*>(binOp->getRight()) ||
                dynamic_cast<StringNode*>(binOp->getRight()))
            {
                rightIsSimple = true;
            }
        }

        if (!rightIsSimple)
        {
            return false;
        }

        if (!willUseOperatorOverloading && rightType == value::ValueType::OBJECT && !usePrimitiveStringConcat)
        {
            std::string rightClassName = inferExpressionClassName(binOp->getRight());
            if (rightClassName == "Int" || rightClassName == "Float" ||
                rightClassName == "Bool" || rightClassName == "String")
            {
                if (outLeftClassName.empty())
                {
                    outLeftClassName = rightClassName;
                }
                willUseOperatorOverloading = true;
            }
        }

        return willUseOperatorOverloading;
    }

    value::ValueType TypeInferenceEngine::inferOverloadedBinaryType(token::TokenType op) const
    {
        // equals() returns primitive bool by Object-interface contract
        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS)
        {
            return value::ValueType::BOOL;
        }

        // lessThan/greaterThan etc. return Bool objects
        if (op == token::TokenType::LESS || op == token::TokenType::GREATER ||
            op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS)
        {
            return value::ValueType::OBJECT;
        }

        return value::ValueType::OBJECT;
    }

    value::ValueType TypeInferenceEngine::inferArithmeticBinaryType(
        token::TokenType op,
        value::ValueType leftType,
        value::ValueType rightType) const
    {
        if (op == token::TokenType::PLUS &&
            (leftType == value::ValueType::STRING || rightType == value::ValueType::STRING)) {
            return value::ValueType::STRING;
        }

        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE) {
            if (leftType == value::ValueType::FLOAT || rightType == value::ValueType::FLOAT) {
                return value::ValueType::FLOAT;
            }
            if (leftType == value::ValueType::INT && rightType == value::ValueType::INT) {
                return value::ValueType::INT;
            }
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferBitwiseBinaryType(
        token::TokenType op,
        value::ValueType leftType,
        value::ValueType rightType) const
    {
        if ((op == token::TokenType::MODULO ||
             op == token::TokenType::BITWISE_AND || op == token::TokenType::BITWISE_OR ||
             op == token::TokenType::BITWISE_XOR || op == token::TokenType::LEFT_SHIFT ||
             op == token::TokenType::RIGHT_SHIFT) &&
            leftType == value::ValueType::INT && rightType == value::ValueType::INT) {
            return value::ValueType::INT;
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferBinaryOperationType(ast::BinaryOpNode* binOp) const
    {
        auto leftType = inferExpressionType(binOp->getLeft());
        auto rightType = inferExpressionType(binOp->getRight());
        auto op = binOp->getOperator();

        // Logical ops always return primitive bool (Bool objects auto-unboxed)
        if (op == token::TokenType::AND || op == token::TokenType::OR) {
            return value::ValueType::BOOL;
        }

        std::string leftClassName;
        if (tryBinaryOperatorOverload(binOp, leftType, rightType, leftClassName))
        {
            return inferOverloadedBinaryType(op);
        }

        auto arith = inferArithmeticBinaryType(op, leftType, rightType);
        if (arith != value::ValueType::VOID) {
            return arith;
        }

        auto bitwise = inferBitwiseBinaryType(op, leftType, rightType);
        if (bitwise != value::ValueType::VOID) {
            return bitwise;
        }

        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
            op == token::TokenType::LESS || op == token::TokenType::GREATER ||
            op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            return value::ValueType::BOOL;
        }

        return value::ValueType::VOID;
    }
}
