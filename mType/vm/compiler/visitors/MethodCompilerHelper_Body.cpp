#include "MethodCompilerHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../MethodSignature.hpp"
#include "../validation/ReturnPathValidator.hpp"
#include "../types/TypeNameValidator.hpp"

namespace vm::compiler::visitors
{
    MethodCompilerHelper::MethodParameters MethodCompilerHelper::collectMethodParameters(ast::MethodNode* node, bool isStatic)
    {
        MethodParameters result;

        std::vector<std::string> validGenericParams;

        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            for (const auto& param : ctx.currentClassNode->getGenericParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        if (node->isGeneric())
        {
            for (const auto& param : node->getGenericTypeParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        auto genericParams = node->getGenericParameters();

        // 'this' is implicitly the first parameter for instance methods.
        if (!isStatic)
        {
            result.paramNames.push_back("this");
            result.paramTypes.push_back(ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "object");
            result.paramNullable.push_back(false);
        }

        for (const auto& param : genericParams)
        {
            result.paramNames.push_back(param.first);
            // Strip '?' — nullability is tracked separately.
            std::string paramTypeStr = param.second->toString();
            bool paramIsNullable = param.second->isNullable();
            paramTypeStr = ::types::TypeConversionUtils::stripNullable(paramTypeStr);
            result.paramTypes.push_back(paramTypeStr);
            result.paramNullable.push_back(paramIsNullable);

            if (!vm::compiler::types::isValidTypeName(paramTypeStr, validGenericParams, *ctx.env))
            {
                throw errors::TypeException(
                    "Undefined type '" + paramTypeStr + "' in parameter '" + param.first + "'. " +
                    "Type must be a primitive, declared generic parameter, or existing class/interface.",
                    node->getLocation()
                );
            }
        }

        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType) {
            result.returnTypeStr = genericReturnType->toString();
        } else {
            result.returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(node->getReturnType());
        }

        if (!vm::compiler::types::isValidTypeName(result.returnTypeStr, validGenericParams, *ctx.env))
        {
            throw errors::TypeException(
                "Undefined type '" + result.returnTypeStr + "' in return type. " +
                "Type must be a primitive, declared generic parameter, or existing class/interface.",
                node->getLocation()
            );
        }

        return result;
    }

    MethodCompilerHelper::MethodBodyInfo MethodCompilerHelper::compileMethodBodyWithFrame(ast::MethodNode* node, const MethodParameters& params,
                                                                                          bool isStatic, const std::string& qualifiedMethodName)
    {
        // qualifiedMethodName (with signature for overloads) must match the
        // name used at runtime so exception tables register correctly.
        ctx.functionFrameManager.enterFunctionFrame(qualifiedMethodName,
                                                    params.returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    node->getIsAsync());
        ctx.variableTracker.beginScope();

        if (!isStatic)
        {
            ctx.variableTracker.declareLocal("this", value::ValueType::OBJECT,
                                             ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "");
        }

        auto genericParams = node->getGenericParameters();
        for (const auto& param : genericParams)
        {
            if (param.second->isGenericParameter())
            {
                // Generic type parameter (T, E, etc.) — treat as object.
                // Strip '?' from className; nullability tracked separately.
                std::string paramClassName = ::types::TypeConversionUtils::stripNullable(param.second->toString());
                ctx.variableTracker.declareLocal(param.first, value::ValueType::OBJECT,
                                                 paramClassName, param.second->isNullable());
            }
            else
            {
                value::ValueType concreteType = param.second->getConcreteType();
                std::string className = (concreteType == value::ValueType::OBJECT)
                    ? ::types::TypeConversionUtils::stripNullable(param.second->toString()) : "";
                ctx.variableTracker.declareLocal(param.first, concreteType, className,
                                                 param.second->isNullable());
            }
        }

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        auto* body = node->getBodyPtr();
        if (body)
        {
            std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() + "::" : "";
            std::string methodName = className + node->getName();

            validation::ReturnPathValidator::validateMethodReturns(
                body,
                node->getReturnType(),
                params.returnTypeStr,
                methodName,
                node->getLocation()
            );
        }

        // MYT-274 Phase 2: structural-equality fast emit. For a synthesized
        // hashCode/equals on an int-only no-parent class, emit a single fused
        // opcode and skip body compilation. The synthesizer still generates
        // an AST body as fallback for ineligible cases (super-compose,
        // non-int fields).
        bool fastEmitted = tryEmitStructuralFastBody(node);

        if (body && !fastEmitted)
        {
            body->accept(ctx.visitor);
        }

        bool isVoidMethod = (node->getReturnType() == value::ValueType::VOID);
        bool isAsyncVoidMethod = (node->getIsAsync() && params.returnTypeStr == "Promise<void>");

        if (isVoidMethod || isAsyncVoidMethod)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        size_t localCount = ctx.functionFrameManager.getLocalCount();

        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        return MethodBodyInfo{ localCount, localVarNames };
    }

    void MethodCompilerHelper::finalizeMethodCompilation(ast::MethodNode* node, const MethodParameters& params,
                                                          size_t methodStart, size_t skipJump, const MethodBodyInfo& bodyInfo, bool isStatic)
    {
        ctx.emitter.patchJump(skipJump);

        // Route through MethodSignature so the mangled name stays in lockstep
        // with ClassRegistrar's MYT-279 pre-registration. Skip "this" for
        // instance methods — added implicitly during compile.
        std::string qualifiedMethodName = node->getName();
        if (ctx.currentClassNode)
        {
            std::vector<std::string> sigTypes(
                params.paramTypes.begin() + (isStatic ? 0 : 1),
                params.paramTypes.end());
            qualifiedMethodName = vm::MethodSignature(node->getName(), sigTypes)
                .toMangledName(ctx.currentClassNode->getClassName(), isStatic);
        }

        // Preserve exception table built during body compilation.
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(qualifiedMethodName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = qualifiedMethodName;
        metadata.startOffset = methodStart;
        metadata.instructionCount = ctx.program.getCurrentOffset() - methodStart;
        metadata.localCount = bodyInfo.localCount;
        metadata.parameterCount = params.paramNames.size();
        metadata.parameterNames = params.paramNames;
        metadata.parameterTypes = params.paramTypes;
        metadata.parameterNullable = params.paramNullable;
        metadata.returnType = params.returnTypeStr;
        metadata.isStatic = isStatic;
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = bodyInfo.localVarNames;

        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }
        }

        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(qualifiedMethodName, metadata);
    }

    // MYT-274 Phase 2: emit a single fused STRUCT_HASH_INT or STRUCT_EQ_INT
    // opcode for synthesized hashCode/equals on a class that is the
    // synthesizer's int-only no-parent shape (every own field is a concrete
    // int, class has no in-program parent). Slot indices are derived from
    // AST field-declaration order (non-static instance fields). This matches
    // ClassDefinition's own fieldIndexMap construction order in
    // ClassRegistrar, since fields are registered in declaration order.
    bool MethodCompilerHelper::tryEmitStructuralFastBody(ast::MethodNode* node)
    {
        if (!node || !node->isSynthetic()) return false;
        if (!ctx.currentClassNode) return false;

        const std::string& methodName = node->getName();
        const bool isHash = (methodName == "hashCode" && node->getParameterCount() == 0);
        const bool isEq = (methodName == "equals" && node->getParameterCount() == 1);
        if (!isHash && !isEq) return false;

        // Skip when there's a user-defined parent class. The synthesizer's
        // super-compose path needs SuperMethodCallNode dispatch which the
        // fused opcode doesn't implement; fall through to AST body.
        if (ctx.currentClassNode->hasParentClass())
        {
            const std::string& parentName = ctx.currentClassNode->getParentClassName();
            if (!parentName.empty() && parentName != "Object")
            {
                return false;
            }
        }

        // All own instance fields must be int; record their slot indices.
        // Static fields don't get slots.
        std::vector<size_t> slotIndices;
        size_t slotCounter = 0;
        for (const auto& fieldAst : ctx.currentClassNode->getFields())
        {
            auto* field = dynamic_cast<ast::nodes::classes::FieldNode*>(fieldAst.get());
            if (!field) continue;
            if (field->getIsStatic()) continue;

            auto genericType = field->getGenericType();
            if (!genericType) return false;
            if (genericType->isParameterized()) return false;
            if (genericType->isNullable()) return false;
            if (genericType->isGenericParameter()) return false;
            if (genericType->getConcreteType() != value::ValueType::INT) return false;

            slotIndices.push_back(slotCounter);
            ++slotCounter;
        }
        if (slotIndices.empty()) return false;

        // Phase 2 v2: fused opcodes have a JIT helper-call emit path
        // (jit_struct_hash_int / jit_struct_eq_int) plus an interpreter
        // executor, so emit them for any int-only-no-parent class. The body
        // shrinks from ~12-25 generic ops (Phase 1 Horner expression) to 2-3
        // ops (LOAD_LOCAL + fused op + RETURN_VALUE), well under
        // INLINE_SIZE_LIMIT, and the inliner picks them up at MONO/POLY call
        // sites in HashMap.put/containsKey/get. The helper does field reads
        // + Horner accumulation in a single C++ loop, avoiding the per-field
        // operand-stack churn of the inlined Horner expression.

        if (isHash)
        {
            // STRUCT_HASH_INT operands: [fieldCount, slot0, slot1, ...].
            // Expects `this` on the stack.
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
            std::vector<uint64_t> operands;
            operands.reserve(1 + slotIndices.size());
            operands.push_back(static_cast<uint64_t>(slotIndices.size()));
            for (size_t s : slotIndices) operands.push_back(static_cast<uint64_t>(s));
            ctx.program.emit(bytecode::OpCode::STRUCT_HASH_INT, operands);
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            return true;
        }

        // isEq: STRUCT_EQ_INT operands: [ownerClassNameIdx, fieldCount, slot...].
        // Stack: ..., this, other.
        const size_t classNameIdx = ctx.program.getConstantPool().addString(
            ctx.currentClassNode->getClassName());

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 1u, node);
        std::vector<uint64_t> operands;
        operands.reserve(2 + slotIndices.size());
        operands.push_back(static_cast<uint64_t>(classNameIdx));
        operands.push_back(static_cast<uint64_t>(slotIndices.size()));
        for (size_t s : slotIndices) operands.push_back(static_cast<uint64_t>(s));
        ctx.program.emit(bytecode::OpCode::STRUCT_EQ_INT, operands);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        return true;
    }
}
