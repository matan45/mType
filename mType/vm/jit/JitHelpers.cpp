#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "ic/InlineCacheTable.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../gc/GC.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/StringPool.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include <vector>
#include <new>
#include <memory>

namespace vm::jit
{
    extern "C" {

        // --- Phase 2: Int/Bool support ---

        int64_t jit_unbox_int(const value::Value* val)
        {
            if (std::holds_alternative<int64_t>(*val))
            {
                return std::get<int64_t>(*val);
            }
            if (std::holds_alternative<bool>(*val))
            {
                return std::get<bool>(*val) ? 1 : 0;
            }
            return 0;
        }

        void jit_set_return_int(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        void jit_set_return_bool(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = (val != 0);
            ctx->hasReturnValue = true;
        }

        void jit_gc_safepoint()
        {
            gc::GC::maybeCollect();
        }

        // --- Phase 3: Float support ---

        float jit_unbox_float(const value::Value* val)
        {
            if (std::holds_alternative<float>(*val))
            {
                return std::get<float>(*val);
            }
            if (std::holds_alternative<int64_t>(*val))
            {
                return static_cast<float>(std::get<int64_t>(*val));
            }
            return 0.0f;
        }

        void jit_set_return_float(JitContext* ctx, float val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        // --- Phase 3: Boxing helpers ---

        void jit_box_int(value::Value* dest, int64_t val)
        {
            *dest = val;
        }

        void jit_box_float(value::Value* dest, float val)
        {
            *dest = val;
        }

        void jit_box_bool(value::Value* dest, int64_t val)
        {
            *dest = (val != 0);
        }

        void jit_box_null(value::Value* dest)
        {
            *dest = std::monostate{};
        }

        // --- Phase 4: Boxed value lifecycle ---

        void jit_locals_init(value::Value* base, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                new (&base[i]) value::Value(std::monostate{});
            }
        }

        void jit_locals_cleanup(value::Value* base, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                std::destroy_at(&base[i]);
            }
        }

        void jit_value_destroy(value::Value* dest)
        {
            std::destroy_at(dest);
            new (dest) value::Value(std::monostate{});
        }

    } // extern "C"

    // --- Phase 3: Call dispatch ---
    // Outside extern "C" because these throw C++ exceptions.

    static void performGenericBinop(value::Value* result,
                                     const value::Value* left,
                                     const value::Value* right,
                                     char op)
    {
        // Integer + Integer
        if (std::holds_alternative<int64_t>(*left) && std::holds_alternative<int64_t>(*right))
        {
            int64_t l = std::get<int64_t>(*left);
            int64_t r = std::get<int64_t>(*right);
            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    if (r == 0) throw errors::RuntimeException("Modulo by zero");
                    *result = l % r; return;
            }
        }

        // Float + Float or Int + Float or Float + Int
        bool leftIsNumeric = std::holds_alternative<float>(*left) || std::holds_alternative<int64_t>(*left);
        bool rightIsNumeric = std::holds_alternative<float>(*right) || std::holds_alternative<int64_t>(*right);

        if (leftIsNumeric && rightIsNumeric)
        {
            float l = std::holds_alternative<float>(*left)
                ? std::get<float>(*left)
                : static_cast<float>(std::get<int64_t>(*left));
            float r = std::holds_alternative<float>(*right)
                ? std::get<float>(*right)
                : static_cast<float>(std::get<int64_t>(*right));

            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    throw errors::RuntimeException("Modulo not supported for float");
            }
        }

        throw errors::RuntimeException("Unsupported operand types for arithmetic");
    }

    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount)
    {
        const std::string& funcName = ctx->program->getConstantPool().getString(nameIndex);

        // 1. Check JIT cache - JIT->JIT direct call
        if (ctx->jitCodeCache)
        {
            auto jitFn = ctx->jitCodeCache->lookup(funcName);
            if (jitFn)
            {
                JitContext nestedCtx{};
                nestedCtx.args = ctx->callArgs;
                nestedCtx.argCount = argCount;
                nestedCtx.hasReturnValue = false;
                nestedCtx.program = ctx->program;
                nestedCtx.stackManager = ctx->stackManager;
                nestedCtx.environment = ctx->environment;
                nestedCtx.vm = ctx->vm;
                nestedCtx.jitCodeCache = ctx->jitCodeCache;

                jitFn(&nestedCtx);

                ctx->returnValue = nestedCtx.returnValue;
                ctx->hasReturnValue = nestedCtx.hasReturnValue;
                return;
            }
        }

        // 2. Check native registry
        auto nativeRegistry = ctx->environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(funcName))
        {
            auto nativeFn = nativeRegistry->findNativeFunction(funcName);
            if (nativeFn)
            {
                std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
                value::Value result = nativeFn(argVec);
                ctx->returnValue = result;
                ctx->hasReturnValue = true;
                return;
            }
        }

        // 3. Fall back to interpreter
        if (ctx->vm)
        {
            std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
            ctx->returnValue = ctx->vm->callFunctionFromJit(funcName, argVec);
            ctx->hasReturnValue = true;
            return;
        }

        throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
    }

    // --- Phase 3: Generic arithmetic helpers ---

    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '+');
    }

    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '-');
    }

    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '*');
    }

    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '/');
    }

    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '%');
    }

    void jit_throw_div_by_zero()
    {
        throw errors::RuntimeException("Division by zero");
    }

    // --- Phase 4: Boxed value helpers ---

    void jit_value_copy(value::Value* dest, const value::Value* src)
    {
        *dest = *src;
    }

    void jit_set_return_boxed(JitContext* ctx, const value::Value* val)
    {
        ctx->returnValue = *val;
        ctx->hasReturnValue = true;
    }

    void jit_value_swap(value::Value* a, value::Value* b)
    {
        value::Value temp = std::move(*a);
        *a = std::move(*b);
        *b = std::move(temp);
    }

    // --- Phase 4A: String support ---

    void jit_push_string(value::Value* dest,
                          const vm::bytecode::BytecodeProgram* prog,
                          uint32_t constIndex)
    {
        const std::string& str = prog->getConstantPool().getString(constIndex);
        auto& pool = value::StringPool::getInstance();
        *dest = pool.intern(str);
    }

    // --- Phase 4B: Object field access ---

    void jit_get_field(value::Value* dest, const value::Value* object,
                        const vm::bytecode::BytecodeProgram* prog,
                        uint32_t fieldNameIndex)
    {
        if (std::holds_alternative<std::nullptr_t>(*object) ||
            std::holds_alternative<std::monostate>(*object))
        {
            throw errors::NullPointerException("Cannot access field on null");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
        const std::string& fieldName = prog->getConstantPool().getString(fieldNameIndex);
        value::Value fieldValue = instance->getFieldValue(fieldName);
        *dest = fieldValue;
    }

    void jit_set_field(value::Value* destValue, const value::Value* object,
                        const value::Value* newValue,
                        const vm::bytecode::BytecodeProgram* prog,
                        uint32_t fieldNameIndex)
    {
        if (std::holds_alternative<std::nullptr_t>(*object) ||
            std::holds_alternative<std::monostate>(*object))
        {
            throw errors::NullPointerException("Cannot set field on null");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
        const std::string& fieldName = prog->getConstantPool().getString(fieldNameIndex);
        instance->setField(fieldName, *newValue);
        *destValue = *newValue;
    }

    // --- Phase 4C: Array operations ---

    void jit_new_array(value::Value* dest, JitContext* ctx,
                        uint32_t typeIndex, int64_t size)
    {
        const std::string& elementTypeName = ctx->program->getConstantPool().getString(typeIndex);
        value::ValueType elemType = types::TypeConversionUtils::stringToValueType(elementTypeName);
        auto array = std::make_shared<value::NativeArray>(
            static_cast<size_t>(size), elemType, elementTypeName);
        *dest = array;
    }

    void jit_array_get(value::Value* dest, const value::Value* array,
                        int64_t index)
    {
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
        {
            auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
            *dest = arr->getUnchecked(index);
            return;
        }
        throw errors::RuntimeException("ARRAY_GET on non-array value");
    }

    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue)
    {
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
        {
            auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
            arr->setUnchecked(index, *newValue);
            return;
        }
        throw errors::RuntimeException("ARRAY_SET on non-array value");
    }

    int64_t jit_array_length(const value::Value* array)
    {
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
        {
            auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
            return static_cast<int64_t>(arr->size());
        }
        throw errors::RuntimeException("ARRAY_LENGTH on non-array value");
    }

    // --- Phase 4D: Method calls ---

    void jit_call_static(JitContext* ctx, uint32_t nameIndex, size_t argCount)
    {
        // Static methods use qualified names in the constant pool,
        // same dispatch as regular function calls.
        jit_call_function(ctx, nameIndex, argCount);
    }

    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount)
    {
        const std::string& methodName = ctx->program->getConstantPool().getString(methodNameIndex);

        // Object is in callArgs[0], args in callArgs[1..argCount]
        value::Value& objectValue = ctx->callArgs[0];

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

            // Collect method arguments (skip callArgs[0] which is the object)
            std::vector<value::Value> args;
            for (size_t i = 1; i <= argCount; i++)
            {
                args.push_back(ctx->callArgs[i]);
            }

            // Delegate to VM's invokeMethod which handles method resolution
            if (ctx->vm)
            {
                ctx->returnValue = ctx->vm->invokeMethod(instance, methodName, args);
                ctx->hasReturnValue = true;
                return;
            }
        }

        throw errors::RuntimeException("JIT: cannot call method '" + methodName + "'");
    }

    // --- Phase 4E: Type operations ---

    int64_t jit_instanceof(const value::Value* val,
                            const vm::bytecode::BytecodeProgram* prog,
                            uint32_t typeIndex)
    {
        const std::string& typeName = prog->getConstantPool().getString(typeIndex);

        if (typeName == "Int" || typeName == "int")
            return std::holds_alternative<int64_t>(*val) ? 1 : 0;
        if (typeName == "Float" || typeName == "float")
            return std::holds_alternative<float>(*val) ? 1 : 0;
        if (typeName == "Bool" || typeName == "bool")
            return std::holds_alternative<bool>(*val) ? 1 : 0;
        if (typeName == "String" || typeName == "string")
            return (std::holds_alternative<std::string>(*val) ||
                    std::holds_alternative<value::InternedString>(*val)) ? 1 : 0;

        // Object instance check with hierarchy
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val))
        {
            auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val);
            auto classDef = instance->getClassDefinition();
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
            if (std::holds_alternative<int64_t>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<float>(*src))
                { *dest = static_cast<int64_t>(std::get<float>(*src)); return; }
            if (std::holds_alternative<bool>(*src))
                { *dest = std::get<bool>(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (targetType == "Float" || targetType == "float")
        {
            if (std::holds_alternative<float>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<int64_t>(*src))
                { *dest = static_cast<float>(std::get<int64_t>(*src)); return; }
        }
        else if (targetType == "Bool" || targetType == "bool")
        {
            if (std::holds_alternative<bool>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<int64_t>(*src))
                { *dest = (std::get<int64_t>(*src) != 0); return; }
        }

        // Object cast or pass-through (type system should have validated at compile time)
        *dest = *src;
    }

    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount)
    {
        const std::string& className = ctx->program->getConstantPool().getString(classIndex);
        std::vector<value::Value> args(ctx->callArgs, ctx->callArgs + argCount);

        if (ctx->vm)
        {
            *dest = ctx->vm->createObject(className, args);
            return;
        }

        throw errors::RuntimeException("JIT: cannot create object '" + className + "'");
    }

    // --- Phase 5: OSR helpers ---

    extern "C" {

        void jit_osr_write_local(JitContext* ctx, size_t slot, const value::Value* val)
        {
            if (ctx->osrOutputLocals && slot < ctx->osrLocalCount)
            {
                ctx->osrOutputLocals[slot] = *val;
            }
        }

        void jit_osr_exit(JitContext* ctx, uint64_t exitOffset)
        {
            ctx->osrExitOffset = static_cast<size_t>(exitOffset);
            ctx->osrExited = true;
        }

    } // extern "C"

    void jit_osr_deoptimize(JitContext* ctx, uint64_t bytecodeOffset)
    {
        throw OSRDeoptException(static_cast<size_t>(bytecodeOffset));
    }

    // --- Phase 7: IC-aware field access ---

    void jit_get_field_ic(value::Value* dest, const value::Value* object,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex)
    {
        using namespace vm::jit::ic;

        if (std::holds_alternative<std::nullptr_t>(*object) ||
            std::holds_alternative<std::monostate>(*object))
        {
            throw errors::NullPointerException("Cannot access field on null");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
        auto* classDef = instance->getClassDefinition().get();

        if (ctx->icTable)
        {
            FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);

            if (cache.state == ICState::MONOMORPHIC ||
                cache.state == ICState::POLYMORPHIC)
            {
                const FieldICEntry* entry = cache.lookup(classDef);
                if (entry && entry->fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                        instance->ensureFieldVector();
                    *dest = instance->getFieldByIndex(entry->fieldIndex);
                    return;
                }
            }

            const std::string& fieldName =
                ctx->program->getConstantPool().getString(fieldNameIndex);
            *dest = instance->getFieldValue(fieldName);

            if (cache.state != ICState::MEGAMORPHIC)
            {
                size_t fieldIndex = classDef->getFieldIndex(fieldName);
                if (fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                        instance->ensureFieldVector();
                    cache.addEntry(classDef, fieldIndex);
                }
            }
            return;
        }

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);
        *dest = instance->getFieldValue(fieldName);
    }

    void jit_set_field_ic(value::Value* destValue, const value::Value* object,
                          const value::Value* newValue,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex)
    {
        using namespace vm::jit::ic;

        if (std::holds_alternative<std::nullptr_t>(*object) ||
            std::holds_alternative<std::monostate>(*object))
        {
            throw errors::NullPointerException("Cannot set field on null");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
        auto* classDef = instance->getClassDefinition().get();

        if (ctx->icTable)
        {
            FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);

            if (cache.state == ICState::MONOMORPHIC ||
                cache.state == ICState::POLYMORPHIC)
            {
                const FieldICEntry* entry = cache.lookup(classDef);
                if (entry && entry->fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                        instance->ensureFieldVector();
                    instance->setFieldByIndex(entry->fieldIndex, *newValue);
                    *destValue = *newValue;
                    return;
                }
            }

            const std::string& fieldName =
                ctx->program->getConstantPool().getString(fieldNameIndex);
            instance->setField(fieldName, *newValue);
            *destValue = *newValue;

            if (cache.state != ICState::MEGAMORPHIC)
            {
                size_t fieldIndex = classDef->getFieldIndex(fieldName);
                if (fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                        instance->ensureFieldVector();
                    cache.addEntry(classDef, fieldIndex);
                }
            }
            return;
        }

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);
        instance->setField(fieldName, *newValue);
        *destValue = *newValue;
    }

    // --- Phase 7: Typed array access ---

    int64_t jit_array_get_int(const value::Value* array, int64_t index)
    {
        auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        value::Value val = arr->getUnchecked(static_cast<size_t>(index));
        return std::get<int64_t>(val);
    }

    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val)
    {
        auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        arr->setUnchecked(static_cast<size_t>(index), value::Value(val));
    }

    // --- Phase 7: SoA-aware array field access ---

    void jit_array_get_field(value::Value* dest, const value::Value* array,
                             int64_t index,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex)
    {
        auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        const std::string& fieldName = prog->getConstantPool().getString(fieldNameIndex);
        size_t idx = static_cast<size_t>(index);

        auto objectArray = arr->getObjectArrayData();
        if (objectArray)
        {
            *dest = objectArray->getFieldUnchecked(idx, fieldName);
            return;
        }

        value::Value element = arr->getUnchecked(idx);
        auto objInstance =
            std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element);
        *dest = objInstance->getFieldValue(fieldName);
    }

    void jit_array_set_field(const value::Value* array, int64_t index,
                             const value::Value* newValue,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex)
    {
        auto arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        const std::string& fieldName = prog->getConstantPool().getString(fieldNameIndex);
        size_t idx = static_cast<size_t>(index);

        auto objectArray = arr->getObjectArrayData();
        if (objectArray)
        {
            objectArray->setFieldUnchecked(idx, fieldName, *newValue);
            return;
        }

        value::Value element = arr->getUnchecked(idx);
        auto objInstance =
            std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element);
        objInstance->setField(fieldName, *newValue);
    }
}
