#include "ExpressionCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../types/NullConditionFacts.hpp"

namespace vm::compiler::visitors
{
    value::Value ExpressionCompiler::compileBinaryOp(ast::BinaryOpNode* node)
    {
        // Try STRING_BUILD optimization for interpolation chains (3+ segments)
        if (tryEmitStringBuild(node))
        {
            return std::monostate{};
        }

        auto op = node->getOperator();

        auto leftType = ctx.typeInference.inferExpressionType(node->getLeft());
        auto leftClassName = ctx.typeInference.inferExpressionClassName(node->getLeft());

        // Handle short-circuit logical operators before normal right-side
        // inference so null guards on the left narrow nullable receivers on
        // the right operand.
        if (op == token::TokenType::AND || op == token::TokenType::OR)
        {
            auto leftFacts = types::analyzeNullCondition(node->getLeft());
            const auto& rightNarrowingFacts =
                op == token::TokenType::AND
                    ? leftFacts.whenTrueNonNull
                    : leftFacts.whenFalseNonNull;

            value::ValueType rightType;
            std::string rightClassName;
            {
                types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                     rightNarrowingFacts);
                rightType = ctx.typeInference.inferExpressionType(node->getRight());
                rightClassName =
                    ctx.typeInference.inferExpressionClassName(node->getRight());
            }

            bool leftIsNull = dynamic_cast<ast::NullNode*>(node->getLeft()) != nullptr;
            bool rightIsNull = dynamic_cast<ast::NullNode*>(node->getRight()) != nullptr;
            ctx.typeValidator.validateBinaryOperation(leftType,
                                                      leftClassName,
                                                      rightType,
                                                      rightClassName,
                                                      op,
                                                      leftIsNull,
                                                      rightIsNull,
                                                      node->getLocation());

            if (op == token::TokenType::AND) {
                // For &&: if left is false, skip right and return false
                node->getLeft()->accept(ctx.visitor);
                if (leftType == value::ValueType::OBJECT && leftClassName == "Bool") {
                    size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                    ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                 static_cast<uint64_t>(getValueIndex),
                                                 0u, node->getLeft());
                }

                size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE_OR_POP, node);

                {
                    types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                         rightNarrowingFacts);
                    node->getRight()->accept(ctx.visitor);
                    if (rightType == value::ValueType::OBJECT && rightClassName == "Bool") {
                        size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                     static_cast<uint64_t>(getValueIndex),
                                                     0u, node->getRight());
                    }
                }

                ctx.emitter.patchJump(jumpOffset);
                return std::monostate{};
            }

            // For ||: if left is true, skip right and return true
            node->getLeft()->accept(ctx.visitor);
            if (leftType == value::ValueType::OBJECT && leftClassName == "Bool") {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(getValueIndex),
                                             0u, node->getLeft());
            }

            size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE_OR_POP, node);

            {
                types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                     rightNarrowingFacts);
                node->getRight()->accept(ctx.visitor);
                if (rightType == value::ValueType::OBJECT && rightClassName == "Bool") {
                    size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                    ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                 static_cast<uint64_t>(getValueIndex),
                                                 0u, node->getRight());
                }
            }

            ctx.emitter.patchJump(jumpOffset);
            return std::monostate{};
        }

        auto rightType = ctx.typeInference.inferExpressionType(node->getRight());
        auto rightClassName = ctx.typeInference.inferExpressionClassName(node->getRight());

        // Operator overloading FIRST for Box types (Int/Float/Bool/String):
        // transforms operators into method calls (e.g. a + b → a.add(b)).
        // Must check BEFORE validation since Box types don't support primitive operators.
        if (tryEmitOperatorOverloading(node, node->getLeft(), node->getRight(), op))
        {
            return std::monostate{};
        }

        // Validate binary operation (only for non-overloaded operators)
        bool leftIsNull = dynamic_cast<ast::NullNode*>(node->getLeft()) != nullptr;
        bool rightIsNull = dynamic_cast<ast::NullNode*>(node->getRight()) != nullptr;
        ctx.typeValidator.validateBinaryOperation(leftType, leftClassName, rightType, rightClassName, op, leftIsNull, rightIsNull, node->getLocation());

        // Compile operands for other operators
        node->getLeft()->accept(ctx.visitor);

        // Auto-unbox left operand for comparison operators if it's Int or Float
        bool isComparisonOp = (op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                              op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS);
        if (isComparisonOp && leftType == value::ValueType::OBJECT)
        {
            std::string leftClassName = ctx.typeInference.inferExpressionClassName(node->getLeft());
            if (leftClassName == "Int" || leftClassName == "Float")
            {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(getValueIndex),
                                             0u, node->getLeft());
            }
        }

        node->getRight()->accept(ctx.visitor);

        // Auto-unbox right operand for comparison operators if it's Int or Float
        if (isComparisonOp && rightType == value::ValueType::OBJECT)
        {
            std::string rightClassName = ctx.typeInference.inferExpressionClassName(node->getRight());
            if (rightClassName == "Int" || rightClassName == "Float")
            {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(getValueIndex),
                                             0u, node->getRight());
            }
        }

        // Emit appropriate opcode with type specialization
        auto spec = emission::ArithmeticSpecialization::NONE;
        if (leftType == value::ValueType::INT && rightType == value::ValueType::INT)
            spec = emission::ArithmeticSpecialization::INT;
        else if (leftType == value::ValueType::FLOAT && rightType == value::ValueType::FLOAT)
            spec = emission::ArithmeticSpecialization::FLOAT;
        bytecode::OpCode opcode = ctx.emitter.getBinaryOpCode(op, spec);
        ctx.emitter.emitWithLocation(opcode, node);

        return std::monostate{};
    }

    bool ExpressionCompiler::tryEmitOperatorOverloading(ast::BinaryOpNode* node,
                                                        ast::ASTNode* left,
                                                        ast::ASTNode* right,
                                                        token::TokenType op)
    {
        // Infer the type of the left operand
        value::ValueType leftType = ctx.typeInference.inferExpressionType(left);
        value::ValueType rightType = ctx.typeInference.inferExpressionType(right);

        // Only use operator overloading if at least one operand is already a Box object.
        // Don't auto-box primitive literals (e.g. 2 * 3 stays primitive) so array indices
        // and similar arithmetic work without needing Box classes.
        //
        // IMPORTANT: For string concatenation, prefer primitive operations over operator
        // overloading because primitive string concat handles objects via toString()
        // without needing a String class.

        std::string leftClassName;
        bool leftNeedsBoxing = false;

        if (leftType == value::ValueType::OBJECT)
        {
            // Left is already an object, get its class name
            leftClassName = ctx.typeInference.inferExpressionClassName(left);
        }
        else
        {
            // Left is a primitive - only allow operator overloading if right is already a Box object
            // This prevents auto-boxing primitive-only operations like 2 + 3
            if (rightType != value::ValueType::OBJECT)
            {
                return false;  // Both operands are primitives, use normal primitive operations
            }

            // SPECIAL CASE: Don't use operator overloading for primitive string concatenation
            // Primitive string + object already works via toString(), no String class needed
            if (leftType == value::ValueType::STRING && op == token::TokenType::PLUS)
            {
                return false;  // Use primitive string concatenation (no String class required)
            }

            // Right is a Box object, so we can auto-box the left literal (non-string only)
            if (dynamic_cast<ast::IntegerNode*>(left))
            {
                leftClassName = "Int";
                leftNeedsBoxing = true;
            }
            else if (dynamic_cast<ast::FloatNode*>(left))
            {
                leftClassName = "Float";
                leftNeedsBoxing = true;
            }
            else if (dynamic_cast<ast::BoolNode*>(left))
            {
                leftClassName = "Bool";
                leftNeedsBoxing = true;
            }
            else
            {
                // Left is a primitive expression (not a literal), can't use operator overloading
                return false;
            }
        }

        // Check if it's a Box type (Int, Float, Bool, String)
        bool isBoxType = (leftClassName == "Int" || leftClassName == "Float" ||
                         leftClassName == "Bool" || leftClassName == "String");
        if (!isBoxType)
        {
            return false;
        }

        // For operator overloading, both operands must be simple (literals or variables).
        // Don't use operator overloading for complex expressions like function calls.
        using namespace ast::nodes::expressions;

        // Check if right is a simple operand (literal, variable, member access, index access, or binary operation)
        bool rightIsSimple = false;
        if (rightType == value::ValueType::OBJECT)
        {
            // Right is an object - check if it's a variable, NewNode, member access, index access, or binary operation
            if (dynamic_cast<ast::VariableNode*>(right) ||
                dynamic_cast<ast::nodes::classes::NewNode*>(right) ||
                dynamic_cast<ast::MemberAccessNode*>(right) ||
                dynamic_cast<IndexAccessNode*>(right) ||
                dynamic_cast<ast::BinaryOpNode*>(right))  // Binary operations on Box types return Box type objects
            {
                rightIsSimple = true;
            }
        }
        else
        {
            // Right is primitive - only use operator overloading if it's a literal
            if (dynamic_cast<IntegerNode*>(right) || dynamic_cast<FloatNode*>(right) ||
                dynamic_cast<BoolNode*>(right) || dynamic_cast<StringNode*>(right))
            {
                rightIsSimple = true;
            }
        }

        if (!rightIsSimple)
        {
            // Right operand is a complex expression (function call, binary op, etc.)
            // Don't use operator overloading - fall back to primitive operations
            return false;
        }

        // Check if right operand has compatible type
        std::string rightClassName;
        if (rightType == value::ValueType::OBJECT)
        {
            rightClassName = ctx.typeInference.inferExpressionClassName(right);
        }
        else
        {
            // Right is primitive - determine its Box type
            if (rightType == value::ValueType::INT) rightClassName = "Int";
            else if (rightType == value::ValueType::FLOAT) rightClassName = "Float";
            else if (rightType == value::ValueType::BOOL) rightClassName = "Bool";
            else if (rightType == value::ValueType::STRING) rightClassName = "String";
        }

        // Operands must be the same Box type (or compatible primitives that will be boxed)
        if (leftClassName != rightClassName)
        {
            // Mixed types - don't use operator overloading
            // Fall back to primitive operations (e.g., primitive string concat)
            return false;
        }

        // Map operator to method name
        std::string methodName;
        switch (op)
        {
            case token::TokenType::PLUS:
                if (leftClassName == "String")
                    methodName = "concat";
                else
                    methodName = "add";
                break;
            case token::TokenType::MINUS:
                methodName = "subtract";
                break;
            case token::TokenType::MULTIPLY:
                methodName = "multiply";
                break;
            case token::TokenType::DIVIDE:
                methodName = "divide";
                break;
            case token::TokenType::MODULO:
                methodName = "modulo";
                break;

            case token::TokenType::EQUALS:
                methodName = "equals";
                break;
            case token::TokenType::NOT_EQUALS:
                // Transform: a != b  →  !(a.equals(b)); negation handled below
                methodName = "equals";
                break;

            case token::TokenType::LESS:
                methodName = "lessThan";
                break;
            case token::TokenType::LESS_EQUALS:
                methodName = "lessThanOrEqual";
                break;
            case token::TokenType::GREATER:
                methodName = "greaterThan";
                break;
            case token::TokenType::GREATER_EQUALS:
                methodName = "greaterThanOrEqual";
                break;

            default:
                return false;
        }

        // Transform operator to method call (a + b  →  a.add(b)).

        // 1. Compile left operand (receiver object), auto-boxing if needed
        if (leftNeedsBoxing)
        {
            left->accept(ctx.visitor);
            size_t classNameIndex = ctx.program.getConstantPool().addString(leftClassName);
            auto boxClassDef = ctx.env->findClass(leftClassName);
            bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
            if (boxIsValue) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                             static_cast<uint64_t>(classNameIndex),
                                             1u, left);
                ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, left);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                             static_cast<uint64_t>(classNameIndex),
                                             1u, left);
            }
        }
        else
        {
            left->accept(ctx.visitor);
        }

        // 2. Compile right operand (method argument) with auto-boxing if needed.
        // For operator overloading, the right operand must match the Box type of the left.
        bool rightIsObject = (rightType == value::ValueType::OBJECT);
        bool rightNeedsBoxing = false;

        if (!rightIsObject)
        {
            // Right is a primitive - check if it matches the Box type we need
            if ((leftClassName == "Int" && rightType == value::ValueType::INT) ||
                (leftClassName == "Float" && rightType == value::ValueType::FLOAT) ||
                (leftClassName == "Bool" && rightType == value::ValueType::BOOL) ||
                (leftClassName == "String" && rightType == value::ValueType::STRING))
            {
                rightNeedsBoxing = true;
            }
        }

        if (rightNeedsBoxing)
        {
            right->accept(ctx.visitor);
            size_t classNameIndex = ctx.program.getConstantPool().addString(leftClassName);
            auto boxClassDefR = ctx.env->findClass(leftClassName);
            bool boxIsValueR = boxClassDefR && boxClassDefR->isValueClass();
            if (boxIsValueR) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                             static_cast<uint64_t>(classNameIndex),
                                             1u, right);
                ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, right);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                             static_cast<uint64_t>(classNameIndex),
                                             1u, right);
            }
        }
        else
        {
            right->accept(ctx.visitor);
        }

        // 3. Emit CALL_METHOD instruction
        size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                     static_cast<uint64_t>(methodNameIndex),
                                     1u,
                                     node);

        // 4. Special handling for NOT_EQUALS: negate the result
        if (op == token::TokenType::NOT_EQUALS)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NOT, node);
        }

        return true;
    }

    void ExpressionCompiler::flattenStringConcat(ast::ASTNode* node,
                                                  std::vector<ast::ASTNode*>& segments)
    {
        auto* binNode = dynamic_cast<ast::BinaryOpNode*>(node);
        if (binNode && binNode->getOperator() == token::TokenType::PLUS)
        {
            // Check if either side is a string → this is a string concat chain
            auto leftType = ctx.typeInference.inferExpressionType(binNode->getLeft());
            auto rightType = ctx.typeInference.inferExpressionType(binNode->getRight());
            bool isStringConcat = (leftType == value::ValueType::STRING ||
                                   rightType == value::ValueType::STRING);

            if (isStringConcat)
            {
                flattenStringConcat(binNode->getLeft(), segments);
                flattenStringConcat(binNode->getRight(), segments);
                return;
            }
        }
        segments.push_back(node);
    }

    bool ExpressionCompiler::tryEmitStringBuild(ast::BinaryOpNode* node)
    {
        if (node->getOperator() != token::TokenType::PLUS)
        {
            return false;
        }

        // Check if this is a string concatenation
        auto leftType = ctx.typeInference.inferExpressionType(node->getLeft());
        auto rightType = ctx.typeInference.inferExpressionType(node->getRight());
        if (leftType != value::ValueType::STRING && rightType != value::ValueType::STRING)
        {
            return false;
        }

        // Flatten the concatenation chain
        std::vector<ast::ASTNode*> segments;
        flattenStringConcat(node, segments);

        // Only use STRING_BUILD for 3+ segments (2 segments is just one ADD)
        if (segments.size() < 3)
        {
            return false;
        }

        // Compile each segment and push onto stack
        for (auto* seg : segments)
        {
            seg->accept(ctx.visitor);
        }

        // Emit STRING_BUILD with segment count
        ctx.emitter.emitWithLocation(bytecode::OpCode::STRING_BUILD,
                                     static_cast<uint64_t>(segments.size()), node);
        return true;
    }
}
