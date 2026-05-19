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

namespace vm::jit
{
    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
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
            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            std::span<const value::Value> args(ctx->callArgs, argCount);

            value::Value direct;
            if (vm::runtime::utils::tryCreatePrimitiveValueObject(
                    className, args, ctx->environment, direct))
            {
                *dest = std::move(direct);
                return;
            }

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
