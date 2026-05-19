#include "TypeExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/TypeArgResolution.hpp"
#include "../../../value/ValueShim.hpp"

namespace vm::runtime
{
    TypeExecutor::TypeExecutor(ExecutionContext& ctx,
                               std::shared_ptr<environment::Environment> env)
        : context(ctx)
        , environment(std::move(env))
    {}

    bool TypeExecutor::checkInstanceOfByName(
        const value::Value& val,
        const std::string& targetTypeName,
        const std::shared_ptr<environment::Environment>& env) {
        if (checkInstanceofPrimitive(val, targetTypeName)) {
            return true;
        }
        return checkInstanceofObject(val, targetTypeName, env.get());
    }

    std::string TypeExecutor::resolveTypeParameter(const std::string& paramName) {
        // Per-frame precedence (typeArgBindings, then receiver class-level
        // bindings) lives in utils::resolveTypeParamInFrame so the JIT helpers
        // can share it. Walk innermost frame outward.
        auto& callStack = context.callStack;
        if (callStack.empty()) {
            throw errors::RuntimeException(
                "Type parameter '" + paramName +
                "' cannot be resolved outside of a function call");
        }

        for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
            if (const auto* resolved = utils::resolveTypeParamInFrame(*it, paramName)) {
                return *resolved;
            }
        }

        throw errors::RuntimeException(
            "Type parameter '" + paramName + "' is not bound in this context");
    }

    void TypeExecutor::handleBindTypeArgs(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-228: stage type-arg bindings for the next CALL_*. Operand layout:
        //   operands[0] = n (pair count)
        //   operands[1 + 3*i + 0] = paramName constant-pool index
        //   operands[1 + 3*i + 1] = TypeArgValueKind (Concrete / ForwardFromCaller)
        //   operands[1 + 3*i + 2] = value constant-pool index
        // Forward-from-caller resolves against the CURRENT top frame's bindings
        // (we haven't pushed the new frame yet). Storing concrete names means
        // the resolver doesn't need a fixpoint walk later.
        if (instr.numOperands() == 0) {
            throw errors::RuntimeException("BIND_TYPE_ARGS: missing operand count");
        }

        // Per-IP fast path: when every binding at this site is Concrete, the
        // resolved (paramName, resolved) pairs are stable. Snapshot once on
        // first execution and bulk-copy thereafter.
        const size_t bindIp = context.instructionPointer;
        if (auto* cached = context.findCachedState(bindIp);
            cached && cached->cachedTypeArgBindingsValid)
        {
            auto& staged = context.pendingTypeArgs.acquireFresh();
            for (const auto& [paramName, resolved] : cached->cachedTypeArgBindings) {
                staged.emplace(paramName, resolved);
            }
            return;
        }

        const auto& constantPool = context.program->getConstantPool();
        const size_t n = static_cast<size_t>(instr.inlineOperands[0]);
        // Variable-arity guard — well-formed bytecode satisfies this; a
        // malformed .mtc would otherwise read past the operand vector.
        if (instr.numOperands() < 1 + 3 * n) {
            throw errors::RuntimeException("BIND_TYPE_ARGS: malformed operands");
        }

        auto& staged = context.pendingTypeArgs.acquireFresh();

        bool allConcrete = true;
        std::vector<std::pair<std::string, std::string>> snapshot;
        snapshot.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const size_t base = 1 + 3 * i;
            const std::string& paramName = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 0)));
            const auto kind = static_cast<bytecode::TypeArgValueKind>(
                static_cast<uint8_t>(instr.operandAt(base + 1)));
            const std::string& rawValue = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 2)));

            std::string resolved;
            if (kind == bytecode::TypeArgValueKind::ForwardFromCaller) {
                // Forward-from-caller depends on caller frame state — site is uncacheable.
                allConcrete = false;
                if (!context.callStack.empty()) {
                    if (const auto* p = utils::resolveTypeParamInFrame(
                            context.callStack.back(), rawValue)) {
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
            auto& slot = context.getOrCreateCachedState(bindIp);
            slot.cachedTypeArgBindings = std::move(snapshot);
            slot.cachedTypeArgBindingsValid = true;
        }
    }

    void TypeExecutor::handleCastTypeParam(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& paramName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        try {
            std::string resolved = resolveTypeParameter(paramName);
            value::Value val = context.stackManager->pop();

            if (resolved == "Int" || resolved == "int") {
                context.stackManager->push(castToInt(val));
            } else if (resolved == "Float" || resolved == "float") {
                context.stackManager->push(castToFloat(val));
            } else if (resolved == "String" || resolved == "string") {
                context.stackManager->push(castToString(val));
            } else if (resolved == "Bool" || resolved == "bool") {
                context.stackManager->push(castToBool(val));
            } else {
                context.stackManager->push(castToObject(val, resolved));
            }
        } catch (const errors::RuntimeException&) {
            // Phase 1 generics: free generic functions don't have reified
            // type parameters at runtime — cast to an erased type parameter
            // is a no-op. Value is already on top of the stack.
        }
    }

    void TypeExecutor::handleCast(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& targetTypeName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value val = context.stackManager->pop();

        // MYT-281: array values flow through Object slots and back.
        // `(Object)arr` is an identity cast; `(T[])obj` is identity when the
        // receiver matches (preserves NativeArray bridge), or an error when
        // the narrow element type disagrees. Never funnel an array through
        // castToObject — that would try to materialize an ObjectInstance.
        if (val.tag() == value::ValueType::ARRAY) {
            if (targetTypeName == "Object") {
                context.stackManager->push(val);
                return;
            }
            const bool targetIsArrayType =
                targetTypeName.size() >= 2 &&
                targetTypeName.compare(targetTypeName.size() - 2, 2, "[]") == 0;
            if (targetIsArrayType) {
                if (value::isNativeArray(val)) {
                    auto arr = value::asNativeArray(val);
                    std::string fullName = arr->getElementTypeName() + "[]";
                    if (fullName == targetTypeName) {
                        context.stackManager->push(val);
                        return;
                    }
                    utils::ErrorLocationHelper::throwUserException(context, environment,
                        "ClassCastException",
                        "cannot cast " + fullName + " to " + targetTypeName);
                }
                // MYT-281: multi-dim primitive arrays carry element type in
                // defaultValue_'s tag and rank in dimensions_. Match strict —
                // both rank and element type must agree. Sub-views inherit
                // defaultValue_ with decremented rank, so a sub-array of
                // `int[][][]` correctly matches `int[][]`.
                std::string multiDimFullName = reconstructMultiArrayTypeName(val);
                if (!multiDimFullName.empty()) {
                    if (multiDimFullName == targetTypeName) {
                        context.stackManager->push(val);
                        return;
                    }
                    utils::ErrorLocationHelper::throwUserException(context, environment,
                        "ClassCastException",
                        "cannot cast " + multiDimFullName + " to " + targetTypeName);
                }
                // Defaults we can't classify (monostate-defaulted arrays not
                // produced by buildMultiArray) keep the prior pass-through.
                context.stackManager->push(val);
                return;
            }
            utils::ErrorLocationHelper::throwUserException(context, environment,
                "ClassCastException",
                "cannot cast array to " + targetTypeName);
        }

        if (targetTypeName == "Int" || targetTypeName == "int") {
            context.stackManager->push(castToInt(val));
        }
        else if (targetTypeName == "Float" || targetTypeName == "float") {
            context.stackManager->push(castToFloat(val));
        }
        else if (targetTypeName == "String" || targetTypeName == "string") {
            context.stackManager->push(castToString(val));
        }
        else if (targetTypeName == "Bool" || targetTypeName == "bool") {
            context.stackManager->push(castToBool(val));
        }
        else {
            context.stackManager->push(castToObject(val, targetTypeName));
        }
    }

    bool TypeExecutor::checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName) {
        if (targetTypeName == "Int" || targetTypeName == "int") {
            return value::isInt(val);
        }
        if (targetTypeName == "Float" || targetTypeName == "float") {
            return value::isFloat(val);
        }
        if (targetTypeName == "Bool" || targetTypeName == "bool") {
            return value::isBool(val);
        }
        if (targetTypeName == "String" || targetTypeName == "string") {
            // MYT-317: STRING_INLINE matches the String type check.
            return value::isAnyString(val);
        }
        return false;
    }
}
