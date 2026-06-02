#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../../../ast/nodes/classes/ThisConstructorCallNode.hpp"

namespace vm::compiler::visitors
{
    value::Value ClassCompiler::compileMemberAccess(ast::MemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();
        bool isStaticAccess = node->getIsStaticAccess();

        if (isStaticAccess)
        {
            // Static field access: ClassName::fieldName
            std::string qualifiedName;
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(node->getObject())) {
                qualifiedName = varNode->getName() + "::" + memberName;
            } else {
                qualifiedName = memberName;
            }
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(fieldNameIndex), node);
        }
        else
        {
            // .length is ALWAYS handled with ARRAY_LENGTH opcode — checked BEFORE the
            // SoA optimization because (1) array[index].length should work for nested
            // arrays and (2) .length is not a field in SoA structure.
            if (memberName == "length")
            {
                node->getObject()->accept(ctx.visitor);
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);
            }
            // array[index].field pattern — SoA optimization via ARRAY_GET_FIELD.
            else if (auto* indexAccessNode = dynamic_cast<ast::IndexAccessNode*>(node->getObject()))
            {
                indexAccessNode->getCollection()->accept(ctx.visitor);
                indexAccessNode->getIndex()->accept(ctx.visitor);

                // Fast path for SoA arrays (direct field access)
                size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
            }
            else
            {
                // Regular instance field access: object.fieldName
                ast::ASTNode* receiverNode = node->getObject();
                bool nonNullReceiver = isReceiverNonNullable(receiverNode);
                bool safe = node->getIsSafe();

                // Safe navigation (obj?.field) permits a nullable receiver and
                // short-circuits to null; only a plain '.' on a nullable
                // receiver is a hard error.
                if (!nonNullReceiver && !safe)
                {
                    throw errors::TypeException(
                        "Cannot access field '" + memberName + "' on nullable receiver. "
                        "Add a null check (if (x != null) { ... }) or change the receiver's declared type to non-nullable.",
                        node->getLocation()
                    );
                }

                receiverNode->accept(ctx.visitor);

                // MYT-374: desugar obj?.field to a DUP + JUMP_IF_NULL short-circuit.
                // On a null receiver: pop it and push null; otherwise fall through
                // to the normal GET_FIELD with a provably non-null receiver.
                size_t safeJumpNull = 0;
                if (safe)
                {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, node);
                    safeJumpNull = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_NULL, node);
                }

                // MYT-212: when the receiver is a variable with a declared
                // class type and that class declares (or inherits) the field,
                // emit the static-binding variant so a child shadowing the
                // field doesn't redirect the read to the most-derived slot.
                std::string staticReceiverClass;
                if (auto* varNode = dynamic_cast<ast::VariableNode*>(receiverNode))
                {
                    const std::string& varName = varNode->getName();
                    std::string localClass = ctx.variableTracker.getLocalClassNameByName(varName);
                    if (!localClass.empty())
                    {
                        staticReceiverClass = ctx.resolveGenericType(localClass);
                    }
                    else
                    {
                        std::string globalClass = ctx.globalRegistry.getClassName(varName);
                        if (!globalClass.empty())
                        {
                            staticReceiverClass = ctx.resolveGenericType(globalClass);
                        }
                    }
                }

                bool emittedTyped = false;
                if (!staticReceiverClass.empty())
                {
                    auto classRegistry = ctx.env->getClassRegistry();
                    if (classRegistry)
                    {
                        std::string baseClass = staticReceiverClass;
                        size_t lt = baseClass.find('<');
                        if (lt != std::string::npos) baseClass = baseClass.substr(0, lt);

                        auto staticDef = classRegistry->findClass(baseClass);
                        if (staticDef && staticDef->getFieldOwnerInHierarchy(memberName, staticDef))
                        {
                            size_t classNameIndex = ctx.program.getConstantPool().addString(baseClass);
                            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD_TYPED,
                                std::vector<uint64_t>{
                                    static_cast<uint64_t>(classNameIndex),
                                    static_cast<uint64_t>(fieldNameIndex)
                                }, node);
                            ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                            emittedTyped = true;
                        }
                    }
                }

                if (!emittedTyped)
                {
                    size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
                    ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                }

                // MYT-374: close the safe-navigation short-circuit.
                if (safe)
                {
                    size_t safeEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
                    ctx.emitter.patchJump(safeJumpNull);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                    ctx.emitter.patchJump(safeEndJump);
                }
            }
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAssignment(ast::MemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        // array[index].field = value — SoA optimization via ARRAY_SET_FIELD.
        auto* objectNode = node->getObject();
        if (auto* indexAccessNode = dynamic_cast<ast::IndexAccessNode*>(objectNode))
        {
            indexAccessNode->getCollection()->accept(ctx.visitor);
            indexAccessNode->getIndex()->accept(ctx.visitor);
            node->getValue()->accept(ctx.visitor);

            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
        }
        else
        {
            // Regular member assignment: object.field = value
            ast::ASTNode* receiverNode = node->getObject();
            bool nonNullReceiver = isReceiverNonNullable(receiverNode);

            if (!nonNullReceiver)
            {
                throw errors::TypeException(
                    "Cannot assign field '" + memberName + "' on nullable receiver. "
                    "Add a null check (if (x != null) { ... }) or change the receiver's declared type to non-nullable.",
                    node->getLocation()
                );
            }

            receiverNode->accept(ctx.visitor);
            node->getValue()->accept(ctx.visitor);

            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
            ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMethodCall(ast::MethodCallNode* node)
    {
        if (node->getIsStaticCall())
        {
            compileStaticMethodCall(node);
        }
        else
        {
            compileInstanceMethodCall(node);
        }
        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperConstructorCall(ast::SuperConstructorCallNode* node)
    {
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                         static_cast<uint64_t>(classNameIndex),
                         static_cast<uint64_t>(arguments.size()), node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileThisConstructorCall(ast::ThisConstructorCallNode* node)
    {
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::THIS_CONSTRUCTOR,
                         static_cast<uint64_t>(classNameIndex),
                         static_cast<uint64_t>(arguments.size()), node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMethodCall(ast::SuperMethodCallNode* node)
    {
        std::string methodName = node->getMethodName();

        // Validate access modifiers for super method calls
        if (ctx.currentClassNode) {
            auto classRegistry = ctx.env->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto method = parentClass->getMethod(methodName);
                        if (method) {
                            if (method->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot call private method '" + methodName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            break;
                        }
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_INVOKE,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(methodNameIndex),
                             static_cast<uint64_t>(arguments.size())
                         }, node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMemberAccess(ast::SuperMemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();

        if (ctx.currentClassNode) {
            auto classRegistry = ctx.env->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto field = parentClass->getField(memberName);
                        if (field) {
                            if (field->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot access private field '" + memberName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            break;
                        }
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t memberNameIndex = ctx.program.getConstantPool().addString(memberName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_GET_FIELD,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(memberNameIndex)
                         }, node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMemberAssignment(ast::SuperMemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        if (ctx.currentClassNode) {
            auto classRegistry = ctx.env->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto field = parentClass->getField(memberName);
                        if (field) {
                            if (field->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot access private field '" + memberName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            break;
                        }
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        node->getValue()->accept(ctx.visitor);

        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t memberNameIndex = ctx.program.getConstantPool().addString(memberName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_SET_FIELD,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(memberNameIndex)
                         }, node);

        return std::monostate{};
    }
}
