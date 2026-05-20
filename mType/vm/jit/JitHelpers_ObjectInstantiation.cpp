#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/ValueObject.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../runtime/utils/TypeArgResolution.hpp"
#include "../runtime/utils/BoxingUtils.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace vm::jit
{
    namespace
    {
        uint64_t makeObjectConstructionCacheKey(uint32_t classIndex, size_t argCount)
        {
            return (static_cast<uint64_t>(classIndex) << 32) |
                   static_cast<uint64_t>(static_cast<uint32_t>(argCount));
        }

        std::vector<std::string> parseTypeArguments(const std::string& typeArgsStr)
        {
            std::vector<std::string> typeArgs;
            std::string current;
            int depth = 0;

            for (char c : typeArgsStr)
            {
                if (c == '<')
                {
                    depth++;
                    current += c;
                }
                else if (c == '>')
                {
                    depth--;
                    current += c;
                }
                else if (c == ',' && depth == 0)
                {
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        typeArgs.push_back(current.substr(start, end - start + 1));
                    }
                    current.clear();
                }
                else
                {
                    current += c;
                }
            }

            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
            {
                typeArgs.push_back(current.substr(start, end - start + 1));
            }

            return typeArgs;
        }

        std::string parseGenericTypeName(
            const std::string& fullClassName,
            environment::Environment* environment,
            std::unordered_map<std::string, std::string>& genericTypeBindings)
        {
            size_t genericStart = fullClassName.find('<');
            if (genericStart == std::string::npos)
            {
                return fullClassName;
            }

            std::string baseClassName = fullClassName.substr(0, genericStart);
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd == std::string::npos || genericEnd <= genericStart)
            {
                return baseClassName;
            }

            auto classDef = environment ? environment->findClass(baseClassName) : nullptr;
            if (!classDef)
            {
                return baseClassName;
            }

            std::string typeArgsStr = fullClassName.substr(
                genericStart + 1, genericEnd - genericStart - 1);
            std::vector<std::string> typeArgs = parseTypeArguments(typeArgsStr);
            const auto& genericParams = classDef->getGenericParameters();
            for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i)
            {
                genericTypeBindings[genericParams[i].name] = typeArgs[i];
            }

            return baseClassName;
        }

        size_t countConstructorsWithArity(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            size_t argCount)
        {
            if (!classDef) return 0;
            size_t matches = 0;
            for (const auto& ctor : classDef->getConstructors())
            {
                if (ctor && ctor->getParameterCount() == argCount)
                    ++matches;
            }
            return matches;
        }

        void resolveObjectConstructionCacheEntry(
            vm::bytecode::BytecodeProgram::ObjectConstructionResolution& entry,
            JitContext* ctx,
            uint32_t classIndex,
            size_t argCount)
        {
            entry.argCount = argCount;
            entry.resolved = true;
            entry.trivialConstructorFastPath = false;
            entry.defaultNoCtorFastPath = false;
            entry.classDef.reset();
            entry.genericTypeBindings.clear();
            entry.trivialFieldIndexAssignments.clear();

            if (!ctx || !ctx->program || !ctx->environment)
                return;

            const std::string& className =
                ctx->program->getConstantPool().getString(classIndex);
            std::unordered_map<std::string, std::string> genericTypeBindings;
            std::string baseClassName =
                parseGenericTypeName(className, ctx->environment, genericTypeBindings);

            auto classDef = ctx->environment->findClass(baseClassName);
            if (!classDef)
                return;

            auto constructor = classDef->findConstructorByTypes(
                std::span<const value::Value>(ctx->callArgs, argCount));
            if (!constructor)
            {
                if (argCount == 0 && classDef->getConstructors().empty())
                {
                    entry.classDef = std::move(classDef);
                    entry.genericTypeBindings = std::move(genericTypeBindings);
                    entry.defaultNoCtorFastPath = true;
                }
                return;
            }

            // Constructor overload selection depends on runtime argument
            // types. Only cache the direct path when this arity is
            // unambiguous; otherwise the existing createObject path keeps
            // overload behavior exact.
            if (countConstructorsWithArity(classDef, argCount) != 1)
                return;

            if (!constructor->isTrivialConstructor())
                return;

            entry.classDef = std::move(classDef);
            entry.genericTypeBindings = std::move(genericTypeBindings);
            entry.trivialFieldIndexAssignments =
                constructor->getTrivialFieldIndexAssignments();
            entry.trivialConstructorFastPath = true;
        }

        bool tryCreateObjectFromCachedConstruction(value::Value* dest,
                                                   JitContext* ctx,
                                                   uint32_t classIndex,
                                                   size_t argCount)
        {
            if (!dest || !ctx || !ctx->program || argCount > UINT32_MAX)
                return false;

            const uint64_t key = makeObjectConstructionCacheKey(classIndex, argCount);
            auto& entry = ctx->program->objectConstructionCache[key];
            if (!entry.resolved)
            {
                resolveObjectConstructionCacheEntry(entry, ctx, classIndex, argCount);
            }

            if (!entry.classDef || entry.argCount != argCount)
                return false;

            if (entry.trivialConstructorFastPath)
            {
                auto instance = entry.genericTypeBindings.empty()
                    ? value::ObjectInstancePool::getInstance().acquire(entry.classDef)
                    : value::ObjectInstancePool::getInstance().acquire(
                          entry.classDef, entry.genericTypeBindings);
                instance->ensureFieldVector();
                for (const auto& [fieldIdx, paramIdx] : entry.trivialFieldIndexAssignments)
                {
                    if (paramIdx < argCount)
                    {
                        instance->setFieldByIndex(fieldIdx, ctx->callArgs[paramIdx]);
                    }
                }
                *dest = value::Value(instance);
                return true;
            }

            if (entry.defaultNoCtorFastPath)
            {
                auto instance = entry.genericTypeBindings.empty()
                    ? value::ObjectInstancePool::getInstance().acquire(entry.classDef)
                    : value::ObjectInstancePool::getInstance().acquire(
                          entry.classDef, entry.genericTypeBindings);
                *dest = value::Value(instance);
                return true;
            }

            return false;
        }
    }

    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            if (tryCreateObjectFromCachedConstruction(dest, ctx, classIndex, argCount))
                return;

            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            // Pass a span over ctx->callArgs directly — no per-call heap alloc.
            std::span<const value::Value> args(ctx->callArgs, argCount);

            if (ctx->vm)
            {
                *dest = ctx->vm->createObject(className, args);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create object '" + className + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_new_value_object(value::Value* dest, JitContext* ctx,
                              uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            std::span<const value::Value> args(ctx->callArgs, argCount);

            value::Value direct;
            if (vm::runtime::utils::tryCreatePrimitiveValueObjectCached(
                    ctx->program, classIndex, args, ctx->environment, direct))
            {
                *dest = std::move(direct);
                return;
            }

            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            if (ctx->vm)
            {
                *dest = ctx->vm->createObject(className, args);
                jit_object_to_value(dest);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create value object '" + className + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // Calls VirtualMachine::createStackObject which hits the trivial-ctor fast
    // path with acquireRaw + push to current frame's stackObjects + STACK_OBJECT
    // emit. Non-trivial ctors fall back to OBJECT (heap) inside createStackObject.
    void jit_new_stack(value::Value* dest, JitContext* ctx,
                        uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            std::span<const value::Value> args(ctx->callArgs, argCount);

            if (ctx->vm)
            {
                *dest = ctx->vm->createStackObject(className, args);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create stack object '" + className + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_object_to_value(value::Value* val)
    {
        if (!value::isObject(*val))
        {
            return;
        }

        auto instance = value::asObject(*val);
        auto classDef = instance->getClassDefinition();
        auto valueObj = std::make_shared<value::ValueObject>(classDef);

        const auto& fieldIndexMap = classDef->getFieldIndexMap();
        for (const auto& [name, index] : fieldIndexMap)
        {
            valueObj->setFieldByIndex(index, instance->getFieldValue(name));
        }

        for (const auto& [param, type] : instance->getGenericTypeBindings())
        {
            valueObj->setGenericTypeBinding(param, type);
        }

        *val = valueObj;
    }

    // Speculative-inline value-class temp materialisation. Mirrors
    // VirtualMachine::callMethodFromJitDirect(const Value&,...).
    void jit_materialise_value_receiver_into_local(value::Value* destLocal,
                                                    const value::Value* sourceReceiver)
    {
        if (!value::isValueObject(*sourceReceiver))
        {
            *destLocal = *sourceReceiver;
            return;
        }

        const auto& valueObj = value::asValueObject(*sourceReceiver);
        if (!valueObj || !valueObj->getClassDefinition())
        {
            // Defensive — receiver claims VALUE_OBJECT but bridge is null.
            // Donate the source bytes; inlined body faults on first field
            // access, matching the slow-path outcome.
            *destLocal = *sourceReceiver;
            return;
        }

        auto tempInstance =
            value::ObjectInstancePool::getInstance().acquire(valueObj->getClassDefinition());
        tempInstance->loadFromValueObject(*valueObj);
        *destLocal = value::Value(tempInstance);
    }
}
