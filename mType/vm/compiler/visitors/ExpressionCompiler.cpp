#include "ExpressionCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "LiteralCompiler.hpp"
#include "ArrayCompiler.hpp"
#include "GenericScopeHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../../../diagnostics/IdentifierEnumerator.hpp"

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
                    ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(nameIndex), node);
                } else if (isLocal) {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(localSlot), node);
                } else {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint64_t>(nameIndex), node);
                }

                // Apply increment/decrement
                bytecode::OpCode opcode = ctx.emitter.getUnaryOpCode(op);
                ctx.emitter.emitWithLocation(opcode, node);

                // Store back
                if (isQualifiedStatic) {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint64_t>(nameIndex), node);
                } else if (isLocal) {
                    // MYT-247: reject ++/-- on a final local at compile time.
                    size_t startSlot = ctx.functionFrameManager.isInFunction()
                                       ? ctx.functionFrameManager.currentFrame().localStartSlot : 0;
                    if (ctx.variableTracker.isLocalFinal(varNode->getName(), startSlot))
                    {
                        throw errors::TypeException(
                            "Cannot increment/decrement final local variable '" + varNode->getName() + "'",
                            node->getLocation()
                        );
                    }

                    // MYT-215: ++/-- mutates the slot.
                    size_t absoluteSlot = localSlot + (ctx.functionFrameManager.isInFunction()
                                          ? ctx.functionFrameManager.currentFrame().localStartSlot : 0);
                    ctx.variableTracker.markVariableAsMutated(absoluteSlot);

                    size_t nameIndex = ctx.program.getConstantPool().addString(varNode->getName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(localSlot), static_cast<uint64_t>(nameIndex), node);
                } else {
                    size_t nameIndex = ctx.program.getConstantPool().addString(varNode->getName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint64_t>(nameIndex), node);
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
                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);

                // Apply increment/decrement
                // Stack: [object, incrementedValue]
                bytecode::OpCode opcode = ctx.emitter.getUnaryOpCode(op);
                ctx.emitter.emitWithLocation(opcode, node);

                // SET_FIELD pops value first (top), then object (below) — already in correct order.
                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);

                return std::monostate{};
            }
        }

        // For other unary operators
        node->getOperand()->accept(ctx.visitor);

        // Auto-unbox Bool objects for NOT operator
        if (op == token::TokenType::NOT)
        {
            value::ValueType operandType = ctx.typeInference.inferExpressionType(node->getOperand());
            if (operandType == value::ValueType::OBJECT)
            {
                std::string operandClassName = ctx.typeInference.inferExpressionClassName(node->getOperand());
                if (operandClassName == "Bool" && ctx.env->findClass("Bool"))
                {
                    size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                    ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                 static_cast<uint64_t>(methodNameIndex),
                                                 0u,
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
        node->getCondition()->accept(ctx.visitor);

        size_t falseJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        node->getTrueExpression()->accept(ctx.visitor);
        size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        ctx.emitter.patchJump(falseJump);
        node->getFalseExpression()->accept(ctx.visitor);

        ctx.emitter.patchJump(endJump);
        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileVariable(ast::VariableNode* node)
    {
        std::string name = node->getName();

        // Check if this is a qualified static access
        if (name.find("::") != std::string::npos) {
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(nameIndex), node);
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
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(localSlot), node);
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
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(nameIndex), node);
                            return std::monostate{};
                        } else if (ctx.inInstanceMethod) {
                            size_t thisSlot = ctx.variableTracker.resolveLocal("this",
                                ctx.functionFrameManager.currentFrame().localStartSlot);
                            if (thisSlot != SIZE_MAX) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(thisSlot), node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
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
                auto currentParentDef = ctx.env->getClassRegistry()->findClass(parentClassName);
                while (currentParentDef) {
                    auto parentField = currentParentDef->getField(name);
                    if (parentField) {
                        if (parentField->isStatic()) {
                            // Access inherited static field using the class where it's defined
                            std::string qualifiedName = currentParentDef->getName() + "::" + name;
                            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(nameIndex), node);
                            return std::monostate{};
                        } else if (ctx.inInstanceMethod) {
                            // Access inherited instance field through 'this'
                            size_t thisSlot = ctx.variableTracker.resolveLocal("this",
                                ctx.functionFrameManager.currentFrame().localStartSlot);
                            if (thisSlot != SIZE_MAX) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(thisSlot), node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
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
                // MYT-35: capture visible identifiers so the diagnostic converter
                // can power "did you mean ...?".
                throw errors::UndefinedException(
                    "Variable '" + name + "' is not defined",
                    node->getLocation(),
                    diagnostics::IdentifierEnumerator::visibleIdentifiers(
                        ctx.env.get()));
            }
            bool inScope = ctx.globalRegistry.isInScope(name, ctx.variableTracker.getCurrentScopeDepth());
            if (!inScope) {
                throw errors::UndefinedException(
                    "Variable '" + name + "' is not defined or is out of scope",
                    node->getLocation(),
                    diagnostics::IdentifierEnumerator::visibleIdentifiers(
                        ctx.env.get()));
            }
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint64_t>(nameIndex), node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileCast(ast::CastExpression* node)
    {
        node->getExpression()->accept(ctx.visitor);

        const auto* targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // IMPORTANT: GenericType::isGenericParameter() is a variant-shape check
        // (variant holds<string>), not a semantic one — it returns true for any
        // bare class name like `Car` or `Box`. We must additionally verify the
        // name is a declared type parameter of the enclosing generic scope,
        // otherwise ordinary casts get misrouted to CAST_TYPEPARAM and the
        // runtime no-op fallback silently swallows real cast errors. Mirrors
        // the guard in compileInstanceOf, broadened to also accept method-level
        // params on free generic functions: FunctionCompiler pushes a self-
        // mapping {T -> T} onto genericTypeBindingStack while compiling the
        // body, and MYT-222's erased-semantics path for `function <T> foo() {
        // (T)x }` depends on hitting CAST_TYPEPARAM here.
        if (targetType->isGenericParameter() && !targetType->isParameterized())
        {
            const std::string& candidate = targetType->getBaseTypeName();
            bool isDeclaredParam = false;

            if (ctx.currentClassNode)
            {
                for (const auto& p : ctx.currentClassNode->getGenericParameters())
                {
                    if (p.name == candidate)
                    {
                        isDeclaredParam = true;
                        break;
                    }
                }
            }

            if (!isDeclaredParam && !ctx.genericTypeBindingStack.empty())
            {
                const auto& bindings = ctx.genericTypeBindingStack.back();
                if (bindings.find(candidate) != bindings.end())
                {
                    isDeclaredParam = true;
                }
            }

            // MYT-228: also accept method/function-level T. MethodCompiler
            // does NOT push a self-mapping onto genericTypeBindingStack
            // (only FunctionCompiler does, for free generic functions),
            // so without this scope walk `(T)a` inside a class's generic
            // method falls through to plain CAST with name "T" and throws
            // at runtime. Mirrors the analogous fall-through in
            // compileInstanceOf.
            if (!isDeclaredParam && isTypeParamInScope(ctx, candidate))
            {
                isDeclaredParam = true;
            }

            if (isDeclaredParam)
            {
                size_t nameIndex = ctx.program.getConstantPool().addString(candidate);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CAST_TYPEPARAM,
                                             static_cast<uint64_t>(nameIndex), node);
                return std::monostate{};
            }
        }

        size_t typeNameIndex = ctx.program.getConstantPool().addString(targetTypeName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CAST, static_cast<uint64_t>(typeNameIndex), node);

        return std::monostate{};
    }

    value::Value ExpressionCompiler::compileInstanceOf(ast::InstanceOfExpression* node)
    {
        node->getExpression()->accept(ctx.visitor);

        const auto* targetType = node->getTargetType();

        // A bare type-parameter RHS (e.g. `obj isClassOf T`) can't be resolved
        // at compile time — `T` is only bound when the enclosing generic class
        // is instantiated. Emit INSTANCEOF_TYPEPARAM so the runtime resolves
        // `T` via the current receiver's generic bindings before dispatching
        // into the normal instanceof machinery.
        //
        // IMPORTANT: GenericType::isGenericParameter() is an implementation
        // check (variant holds<string>), not a semantic check — it returns
        // true for ANY class name like `Dog` or `Box`, not just for actual
        // type parameters. We must additionally verify the name matches a
        // declared type parameter of the enclosing generic scope. Without
        // this guard every `obj isClassOf UserClass` would be routed through
        // the TYPEPARAM path and blow up at runtime.
        if (targetType->isGenericParameter() && !targetType->isParameterized())
        {
            const std::string& candidate = targetType->getBaseTypeName();
            bool isDeclaredParam = false;

            // Check class-level generic parameters first — those carry
            // reified bindings on `this` at runtime and dispatch through
            // INSTANCEOF_TYPEPARAM via TypeExecutor::resolveTypeParameter.
            if (ctx.currentClassNode)
            {
                for (const auto& p : ctx.currentClassNode->getGenericParameters())
                {
                    if (p.name == candidate)
                    {
                        isDeclaredParam = true;
                        break;
                    }
                }
            }

            // MYT-228: also accept method/function-level T. The runtime
            // resolves it via CallFrame::typeArgBindings, populated by a
            // BIND_TYPE_ARGS opcode emitted before the call. Mirrors the
            // analogous fall-through in compileCast.
            if (!isDeclaredParam && isTypeParamInScope(ctx, candidate))
            {
                isDeclaredParam = true;
            }

            if (isDeclaredParam)
            {
                size_t nameIndex = ctx.program.getConstantPool().addString(candidate);
                ctx.emitter.emitWithLocation(bytecode::OpCode::INSTANCEOF_TYPEPARAM,
                                             static_cast<uint64_t>(nameIndex), node);
                return std::monostate{};
            }
        }

        // Concrete RHS — may be raw ("Box") or parameterized ("Box<Int>").
        // GenericType::toString() already emits the canonical parameterized
        // form with ", " spacing, which the runtime reconstruction matches
        // exactly so string equality does the discrimination job.
        std::string targetTypeName = targetType->toString();
        size_t typeNameIndex = ctx.program.getConstantPool().addString(targetTypeName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::INSTANCEOF, static_cast<uint64_t>(typeNameIndex), node);

        return std::monostate{};
    }
}
