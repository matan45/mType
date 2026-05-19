#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include "../runtime/utils/TypeArgResolution.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include <cstdint>

namespace vm::jit
{
    int64_t jit_instanceof(const value::Value* val,
                            const vm::bytecode::BytecodeProgram* prog,
                            uint32_t typeIndex)
    {
        const std::string& typeName = prog->getConstantPool().getString(typeIndex);

        if (typeName == "Int" || typeName == "int")
            return value::isInt(*val) ? 1 : 0;
        if (typeName == "Float" || typeName == "float")
            return value::isFloat(*val) ? 1 : 0;
        if (typeName == "Bool" || typeName == "bool")
            return value::isBool(*val) ? 1 : 0;
        if (typeName == "String" || typeName == "string")
            return (value::isString(*val) ||
                    value::isInternedString(*val)) ? 1 : 0;

        if (value::isAnyObject(*val))
        {
            auto* raw = value::asObjectInstanceRaw(*val);
            auto classDef = raw->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == typeName) return 1;
                classDef = classDef->getParentClass();
            }
        }

        if (value::isValueObject(*val))
        {
            auto valueObj = value::asValueObject(*val);
            auto classDef = valueObj->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == typeName) return 1;
                classDef = classDef->getParentClass();
            }
        }

        return 0;
    }

    void jit_cast(value::Value* dest, const value::Value* src,
                   const vm::bytecode::BytecodeProgram* prog,
                   uint32_t typeIndex)
    {
        const std::string& targetType = prog->getConstantPool().getString(typeIndex);

        if (targetType == "Int" || targetType == "int")
        {
            if (value::isInt(*src))
                { *dest = *src; return; }
            if (value::isFloat(*src))
                { *dest = static_cast<int64_t>(value::asFloat(*src)); return; }
            if (value::isBool(*src))
                { *dest = value::asBool(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (targetType == "Float" || targetType == "float")
        {
            if (value::isFloat(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = static_cast<double>(value::asInt(*src)); return; }
        }
        else if (targetType == "Bool" || targetType == "bool")
        {
            if (value::isBool(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = (value::asInt(*src) != 0); return; }
        }

        *dest = *src;
    }

    // Resolves type-param against the current call stack then delegates to the
    // name-based instanceof. Without this resolution the JIT would match the
    // literal name "T" against class hierarchies and always return 0.
    int64_t jit_instanceof_typeparam(const value::Value* val,
                                      JitContext* ctx,
                                      uint32_t paramNameIndex)
    {
        const std::string& paramName = ctx->program->getConstantPool().getString(paramNameIndex);
        const std::string* resolvedPtr = &paramName;

        if (ctx->vm)
        {
            const auto& callStack = ctx->vm->getCallStack();
            for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
                if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(*it, paramName)) {
                    resolvedPtr = p;
                    break;
                }
            }
        }

        const std::string& resolved = *resolvedPtr;

        if (resolved == "Int" || resolved == "int")
            return value::isInt(*val) ? 1 : 0;
        if (resolved == "Float" || resolved == "float")
            return value::isFloat(*val) ? 1 : 0;
        if (resolved == "Bool" || resolved == "bool")
            return value::isBool(*val) ? 1 : 0;
        if (resolved == "String" || resolved == "string")
            return (value::isString(*val) || value::isInternedString(*val)) ? 1 : 0;

        if (value::isAnyObject(*val))
        {
            auto* raw = value::asObjectInstanceRaw(*val);
            auto classDef = raw->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == resolved) return 1;
                classDef = classDef->getParentClass();
            }
        }

        if (value::isValueObject(*val))
        {
            auto valueObj = value::asValueObject(*val);
            auto classDef = valueObj->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == resolved) return 1;
                classDef = classDef->getParentClass();
            }
        }

        return 0;
    }

    // Stage type-arg bindings for the next CALL_*. Populates the pool-backed
    // pending map directly in-place; mirrors TypeExecutor::handleBindTypeArgs.
    void jit_bind_type_args(JitContext* ctx, uint64_t ip)
    {
        if (!ctx->vm || !ctx->program) return;

        // Per-IP fast path: when every binding at this site is Concrete, the
        // resolved pairs are stable. Bulk-copy from the cached snapshot.
        const size_t bindIp = static_cast<size_t>(ip);
        if (auto* cached = ctx->program->findCachedState(bindIp);
            cached && cached->cachedTypeArgBindingsValid)
        {
            auto& staged = ctx->vm->beginPendingTypeArgs();
            for (const auto& [paramName, resolved] : cached->cachedTypeArgBindings) {
                staged.emplace(paramName, resolved);
            }
            return;
        }

        const auto& instr = ctx->program->getInstructions()[bindIp];
        if (instr.numOperands() == 0) return;
        const auto& constantPool = ctx->program->getConstantPool();
        const size_t n = static_cast<size_t>(instr.inlineOperands[0]);
        if (instr.numOperands() < 1 + 3 * n) return;

        auto& staged = ctx->vm->beginPendingTypeArgs();
        const auto& callStack = ctx->vm->getCallStack();

        bool allConcrete = true;
        std::vector<std::pair<std::string, std::string>> snapshot;
        snapshot.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const size_t base = 1 + 3 * i;
            const std::string& paramName = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 0)));
            const auto kind = static_cast<vm::bytecode::TypeArgValueKind>(
                static_cast<uint8_t>(instr.operandAt(base + 1)));
            const std::string& rawValue = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 2)));

            std::string resolved;
            if (kind == vm::bytecode::TypeArgValueKind::ForwardFromCaller) {
                allConcrete = false;
                if (!callStack.empty()) {
                    if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(
                            callStack.back(), rawValue)) {
                        resolved = *p;
                    } else {
                        resolved = rawValue;
                    }
                } else {
                    resolved = rawValue;
                }
            } else {
                resolved = rawValue;
                if (allConcrete) {
                    snapshot.emplace_back(paramName, resolved);
                }
            }

            staged.emplace(paramName, std::move(resolved));
        }

        if (allConcrete) {
            auto& slot = ctx->program->getOrCreateCachedState(bindIp);
            slot.cachedTypeArgBindings = std::move(snapshot);
            slot.cachedTypeArgBindingsValid = true;
        }
    }

    void jit_cast_typeparam(value::Value* dest, const value::Value* src,
                             JitContext* ctx,
                             uint32_t paramNameIndex)
    {
        const std::string& paramName = ctx->program->getConstantPool().getString(paramNameIndex);
        std::string resolved = paramName;

        if (ctx->vm)
        {
            auto& callStack = ctx->vm->getCallStack();
            for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
                if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(*it, paramName)) {
                    resolved = *p;
                    break;
                }
            }
        }

        if (resolved == "Int" || resolved == "int")
        {
            if (value::isInt(*src))
                { *dest = *src; return; }
            if (value::isFloat(*src))
                { *dest = static_cast<int64_t>(value::asFloat(*src)); return; }
            if (value::isBool(*src))
                { *dest = value::asBool(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (resolved == "Float" || resolved == "float")
        {
            if (value::isFloat(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = static_cast<double>(value::asInt(*src)); return; }
        }
        else if (resolved == "Bool" || resolved == "bool")
        {
            if (value::isBool(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = (value::asInt(*src) != 0); return; }
        }

        // Phase 1 generics runtime erasure: object types pass through.
        *dest = *src;
    }
}
