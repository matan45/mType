#include "ExpressionCompiler.hpp"
#include "LiteralCompiler.hpp"
#include "ArrayCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
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

        // Validate binary operation
        bool leftIsNull = dynamic_cast<ast::NullNode*>(node->getLeft()) != nullptr;
        bool rightIsNull = dynamic_cast<ast::NullNode*>(node->getRight()) != nullptr;
        ctx.typeValidator.validateBinaryOperation(leftType, rightType, op, leftIsNull, rightIsNull, node->getLocation());

        // Handle short-circuit logical operators specially
        if (op == token::TokenType::AND) {
            // For &&: if left is false, skip right and return false
            node->getLeft()->accept(ctx.visitor);
            size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE_OR_POP, node);
            node->getRight()->accept(ctx.visitor);
            ctx.emitter.patchJump(jumpOffset);
            return std::monostate{};
        }

        if (op == token::TokenType::OR) {
            // For ||: if left is true, skip right and return true
            node->getLeft()->accept(ctx.visitor);
            size_t jumpOffset = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE_OR_POP, node);
            node->getRight()->accept(ctx.visitor);
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
            if (!ctx.globalRegistry.isInScope(name, ctx.variableTracker.getCurrentScopeDepth())) {
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
}
