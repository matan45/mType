#include "ExpressionCompiler.hpp"
#include "LiteralCompiler.hpp"
#include "ArrayCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../errors/UndefinedException.hpp"
#include  <iostream>

namespace vm::compiler::visitors
{
    ExpressionCompiler::ExpressionCompiler(CompilerContext& context,
                                          LiteralCompiler& literalComp,
                                          ArrayCompiler& arrayComp)
        : ctx(context)
        , literalCompiler(literalComp)
        , arrayCompiler(arrayComp)
    {
    }

    value::Value ExpressionCompiler::compileBinaryOp(ast::BinaryOpNode* node)
    {
        auto leftType = ctx.typeInference.inferExpressionType(node->getLeft());
        auto rightType = ctx.typeInference.inferExpressionType(node->getRight());
        auto op = node->getOperator();

        // PHASE 4: Try operator overloading FIRST for Box types (Int, Float, Bool, String)
        // This transforms operators into method calls (e.g., a + b → a.add(b))
        // Must check BEFORE validation since Box types don't support primitive operators
        if (tryEmitOperatorOverloading(node, node->getLeft(), node->getRight(), op))
        {
            return std::monostate{};  // Operator overloading handled the compilation
        }

        // Validate binary operation (only for non-overloaded operators)
        bool leftIsNull = dynamic_cast<ast::NullNode*>(node->getLeft()) != nullptr;
        bool rightIsNull = dynamic_cast<ast::NullNode*>(node->getRight()) != nullptr;
        ctx.typeValidator.validateBinaryOperation(leftType, rightType, op, leftIsNull, rightIsNull, node->getLocation());

        // Handle short-circuit logical operators specially
        if (op == token::TokenType::AND) {
            // For &&: if left is false, skip right and return false

            // PHASE 4: Auto-unbox left operand if it's a Bool object
            std::string leftClassName = ctx.typeInference.inferExpressionClassName(node->getLeft());
            node->getLeft()->accept(ctx.visitor);
            if (leftType == value::ValueType::OBJECT && leftClassName == "Bool") {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(getValueIndex),
                                             0u, node->getLeft());
            }

            size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE_OR_POP, node);

            // PHASE 4: Auto-unbox right operand if it's a Bool object
            std::string rightClassName = ctx.typeInference.inferExpressionClassName(node->getRight());
            node->getRight()->accept(ctx.visitor);
            if (rightType == value::ValueType::OBJECT && rightClassName == "Bool") {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(getValueIndex),
                                             0u, node->getRight());
            }

            ctx.emitter.patchJump(jumpOffset);
            return std::monostate{};
        }

        if (op == token::TokenType::OR) {
            // For ||: if left is true, skip right and return true

            // PHASE 4: Auto-unbox left operand if it's a Bool object
            std::string leftClassName = ctx.typeInference.inferExpressionClassName(node->getLeft());
            node->getLeft()->accept(ctx.visitor);
            if (leftType == value::ValueType::OBJECT && leftClassName == "Bool") {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(getValueIndex),
                                             0u, node->getLeft());
            }

            size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE_OR_POP, node);

            // PHASE 4: Auto-unbox right operand if it's a Bool object
            std::string rightClassName = ctx.typeInference.inferExpressionClassName(node->getRight());
            node->getRight()->accept(ctx.visitor);
            if (rightType == value::ValueType::OBJECT && rightClassName == "Bool") {
                size_t getValueIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(getValueIndex),
                                             0u, node->getRight());
            }

            ctx.emitter.patchJump(jumpOffset);
            return std::monostate{};
        }

        // Compile operands for other operators
        node->getLeft()->accept(ctx.visitor);  // Will need delegation
        node->getRight()->accept(ctx.visitor);  // Will need delegation

        // Emit appropriate opcode
        bool typeSpecialized = (leftType == value::ValueType::INT && rightType == value::ValueType::INT);
        bytecode::OpCode opcode = ctx.emitter.getBinaryOpCode(op, typeSpecialized);
        ctx.emitter.emitWithLocation(opcode, node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileUnaryOp(ast::UnaryOpNode* node)
    {
        auto op = node->getOperator();

        // Handle increment/decrement (they modify variables)
        if (op == token::TokenType::INCREMENT || op == token::TokenType::DECREMENT) {
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(node->getOperand())) {
                std::string varName = varNode->getName();

                // Check if it's a qualified static field (ClassName::fieldName)
                bool isQualifiedStatic = (varName.find("::") != std::string::npos);

                // Check if it's a static field of current class
                bool isStaticField = false;
                if (!isQualifiedStatic && ctx.currentClassNode) {
                    for (const auto& field : ctx.currentClassNode->getFields()) {
                        if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                            if (fieldNode->getName() == varName && fieldNode->getIsStatic()) {
                                isStaticField = true;
                                varName = ctx.currentClassNode->getClassName() + "::" + varName;
                                isQualifiedStatic = true;
                                break;
                            }
                        }
                    }
                }

                size_t localSlot = ctx.variableTracker.resolveLocal(varNode->getName(),
                    ctx.functionFrameManager.isInFunction() ?
                    ctx.functionFrameManager.currentFrame().localStartSlot : 0);
                bool isLocal = (localSlot != SIZE_MAX);

                // Load current value
                if (isQualifiedStatic) {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
                } else if (isLocal) {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
                } else {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex), node);
                }

                // Apply increment/decrement
                bytecode::OpCode opcode = ctx.emitter.getUnaryOpCode(op);
                ctx.emitter.emitWithLocation(opcode, node);

                // Store back
                if (isQualifiedStatic) {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
                } else if (isLocal) {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varNode->getName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(localSlot), static_cast<uint32_t>(nameIndex), node);
                } else {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varNode->getName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex), node);
                }

                return std::monostate{};
            }
            // Handle increment/decrement on member access (e.g., this.field++, obj.field--)
            else if (auto* memberNode = dynamic_cast<ast::MemberAccessNode*>(node->getOperand())) {
                std::string fieldName = memberNode->getMemberName();

                // Compile the object expression (e.g., 'this' or 'obj')
                memberNode->getObject()->accept(ctx.visitor);

                // Duplicate object reference for later SET_FIELD
                // Stack: [object, object]
                ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, node);

                // Get current field value
                // Stack: [object, fieldValue]
                size_t fieldNameIndex = ctx.program.getConstantPool().addString(fieldName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);

                // Apply increment/decrement
                // Stack: [object, incrementedValue]
                bytecode::OpCode opcode = ctx.emitter.getUnaryOpCode(op);
                ctx.emitter.emitWithLocation(opcode, node);

                // SET_FIELD pops value first (top), then object (below)
                // Current stack: [object, incrementedValue] - value already on top, object below
                // This is the correct order! No swap needed.
                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);

                return std::monostate{};
            }
        }

        // For other unary operators
        node->getOperand()->accept(ctx.visitor);  // Will need delegation

        // PHASE 4: Auto-unbox Bool objects for NOT operator
        if (op == token::TokenType::NOT)
        {
            value::ValueType operandType = ctx.typeInference.inferExpressionType(node->getOperand());
            if (operandType == value::ValueType::OBJECT)
            {
                std::string operandClassName = ctx.typeInference.inferExpressionClassName(node->getOperand());
                if (operandClassName == "Bool")
                {
                    // Auto-unbox: call getValue() to get primitive bool
                    size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                    ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                 static_cast<uint32_t>(methodNameIndex),
                                                 0u,  // 0 arguments
                                                 node->getOperand());
                }
            }
        }

        bytecode::OpCode opcode = ctx.emitter.getUnaryOpCode(op);
        ctx.emitter.emitWithLocation(opcode, node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileTernaryOp(ast::TernaryOpNode* node)
    {
        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // Jump to false branch if condition is false
        size_t falseJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Compile true branch
        node->getTrueExpression()->accept(ctx.visitor);  // Will need delegation
        size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Compile false branch
        ctx.emitter.patchJump(falseJump);
        node->getFalseExpression()->accept(ctx.visitor);  // Will need delegation

        // Patch end jump
        ctx.emitter.patchJump(endJump);
        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileVariable(ast::VariableNode* node)
    {
        std::string name = node->getName();

        // Check if this is a qualified static access
        if (name.find("::") != std::string::npos) {
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
            return std::monostate{};
        }

        // Check if this is a local variable
        size_t localSlot = SIZE_MAX;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            localSlot = ctx.variableTracker.resolveLocal(name, startSlot);
        } else {
            // Also check for locals at top level (e.g., catch variables in main script)
            localSlot = ctx.variableTracker.resolveLocal(name, 0);
        }

        if (localSlot != SIZE_MAX) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
            return std::monostate{};
        }

        // Check if we're in a class context
        if (ctx.currentClassNode) {
            // First check current class fields
            for (const auto& field : ctx.currentClassNode->getFields()) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                    if (fieldNode->getName() == name) {
                        if (fieldNode->getIsStatic()) {
                            std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + name;
                            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
                            return std::monostate{};
                        } else if (ctx.inInstanceMethod) {
                            size_t thisSlot = ctx.variableTracker.resolveLocal("this",
                                ctx.functionFrameManager.currentFrame().localStartSlot);
                            if (thisSlot != SIZE_MAX) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(thisSlot), node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
                                return std::monostate{};
                            }
                        }
                        break;
                    }
                }
            }

            // Check parent class hierarchy for inherited fields (recursive search)
            if (ctx.currentClassNode->hasParentClass()) {
                std::string parentClassName = ctx.currentClassNode->getParentClassName();

                // Walk up the entire inheritance chain
                auto currentParentDef = ctx.environment->getClassRegistry()->findClass(parentClassName);
                while (currentParentDef) {
                    auto parentField = currentParentDef->getField(name);
                    if (parentField) {
                        if (parentField->isStatic()) {
                            // Access inherited static field using the class where it's defined
                            std::string qualifiedName = currentParentDef->getName() + "::" + name;
                            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
                            return std::monostate{};
                        } else if (ctx.inInstanceMethod) {
                            // Access inherited instance field through 'this'
                            size_t thisSlot = ctx.variableTracker.resolveLocal("this",
                                ctx.functionFrameManager.currentFrame().localStartSlot);
                            if (thisSlot != SIZE_MAX) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(thisSlot), node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
                                return std::monostate{};
                            }
                        }
                    }

                    // Move to next parent in the chain
                    currentParentDef = currentParentDef->getParentClass();
                }
            }
        }

        // Global variable lookup - validate
        bool inLambda = ctx.functionFrameManager.isInLambda();

        if (!inLambda) {
            if (!ctx.globalRegistry.exists(name)) {
                throw errors::UndefinedException("Variable '" + name + "' is not defined", node->getLocation());
            }
            bool inScope = ctx.globalRegistry.isInScope(name, ctx.variableTracker.getCurrentScopeDepth());
            if (!inScope) {
                throw errors::UndefinedException("Variable '" + name + "' is not defined or is out of scope", node->getLocation());
            }
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex), node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileCast(ast::CastExpression* node)
    {
        // Compile the expression to be cast
        node->getExpression()->accept(ctx.visitor);  // Will need delegation

        // Get target type information
        const auto* targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // Store target type name in constant pool
        size_t typeNameIndex = ctx.program.getConstantPool().addString(targetTypeName);

        // Emit CAST instruction
        ctx.emitter.emitWithLocation(bytecode::OpCode::CAST, static_cast<uint32_t>(typeNameIndex), node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileInstanceOf(ast::InstanceOfExpression* node)
    {
        // Compile the expression
        node->getExpression()->accept(ctx.visitor);  // Will need delegation

        // Get target type information
        const auto* targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // Store target type name in constant pool
        size_t typeNameIndex = ctx.program.getConstantPool().addString(targetTypeName);

        // Emit INSTANCEOF instruction
        ctx.emitter.emitWithLocation(bytecode::OpCode::INSTANCEOF, static_cast<uint32_t>(typeNameIndex), node);

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

        // PHASE 4: Only use operator overloading if at least one operand is already a Box object
        // Don't auto-box primitive literals for operator overloading (e.g., 2 * 3 stays primitive)
        // This ensures that expressions like array indices (calcTest[2 * 3][4 * 5]) work without needing Box classes
        //
        // IMPORTANT: For string concatenation, prefer primitive operations over operator overloading
        // because primitive string concat handles objects via toString() without needing String class

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

        // PHASE 4: For operator overloading, both operands must be simple (literals or variables)
        // Don't use operator overloading for complex expressions like function calls
        using namespace ast::nodes::expressions;

        // Check if right is a simple operand (literal, variable, member access, or index access)
        bool rightIsSimple = false;
        if (rightType == value::ValueType::OBJECT)
        {
            // Right is an object - check if it's a variable, NewNode, member access, or index access
            if (dynamic_cast<ast::VariableNode*>(right) ||
                dynamic_cast<ast::nodes::classes::NewNode*>(right) ||
                dynamic_cast<ast::MemberAccessNode*>(right) ||
                dynamic_cast<IndexAccessNode*>(right))
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
            // Arithmetic operators
            case token::TokenType::PLUS:
                if (leftClassName == "String")
                    methodName = "concat";  // String concatenation
                else
                    methodName = "add";     // Int/Float addition
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

            // Equality operators
            case token::TokenType::EQUALS:
                methodName = "equals";
                break;
            case token::TokenType::NOT_EQUALS:
                // Transform: a != b  →  !(a.equals(b))
                // We'll handle this specially below
                methodName = "equals";
                break;

            // Comparison operators
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
                // Operator not supported for overloading
                return false;
        }

        // PHASE 4 OPERATOR OVERLOADING: Transform operator to method call
        // Example: a + b  →  a.add(b)

        // 1. Compile left operand (receiver object), auto-boxing if needed
        if (leftNeedsBoxing)
        {
            // Auto-box the left operand: compile literal then wrap in NEW_OBJECT
            left->accept(ctx.visitor);
            size_t classNameIndex = ctx.program.getConstantPool().addString(leftClassName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint32_t>(classNameIndex),
                                         1u,  // 1 constructor argument
                                         left);
        }
        else
        {
            left->accept(ctx.visitor);
        }

        // 2. Compile right operand (method argument) with auto-boxing if needed
        // For operator overloading, the right operand must match the Box type of the left
        // Check if right needs auto-boxing: it's a primitive that should be a Box type
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
            // Auto-box the right operand: compile it then wrap in NEW_OBJECT
            right->accept(ctx.visitor);
            size_t classNameIndex = ctx.program.getConstantPool().addString(leftClassName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint32_t>(classNameIndex),
                                         1u,  // 1 constructor argument
                                         right);
        }
        else
        {
            // Compile normally (already an object or incompatible type)
            right->accept(ctx.visitor);
        }

        // 3. Emit CALL_METHOD instruction
        size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                     static_cast<uint32_t>(methodNameIndex),
                                     1u,  // 1 argument
                                     node);

        // 4. Special handling for NOT_EQUALS: negate the result
        if (op == token::TokenType::NOT_EQUALS)
        {
            // Emit NOT instruction to negate the equals() result
            ctx.emitter.emitWithLocation(bytecode::OpCode::NOT, node);
        }

        return true;  // Operator overloading was applied
    }
}
