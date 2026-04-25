#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"

namespace vm::jit
{

    void jit_throw_array_oob(int64_t index, int64_t size)
    {
        throw errors::RuntimeException(
            "Array index out of bounds: index " + std::to_string(index) +
            ", size " + std::to_string(size));
    }

    void jit_new_array(value::Value* dest, JitContext* ctx,
                        uint32_t typeIndex, int64_t size)
    {
        const std::string& elementTypeName = ctx->program->getConstantPool().getString(typeIndex);
        value::ValueType elemType = types::TypeConversionUtils::stringToValueType(elementTypeName);
        auto array = std::make_shared<value::NativeArray>(
            static_cast<size_t>(size), elemType, elementTypeName);
        *dest = array;
    }

    // MYT-146: multi-dim array construction. Dimension sizes arrive in
    // ctx->callArgs[0..specifiedDims-1] as raw int64 variants; the emitter
    // marshals them in stack order (receiver-deepest is callArgs[0], matching
    // the interpreter's bottom-to-top dimension order after its reverse).
    void jit_new_array_multi(value::Value* dest, JitContext* ctx,
                              uint32_t typeIndex,
                              uint32_t totalDims,
                              uint32_t specifiedDims)
    {
        if (!ctx->vm)
            throw errors::RuntimeException("JIT: jit_new_array_multi invoked with null VM");

        std::vector<int64_t> dimensions;
        dimensions.reserve(specifiedDims);
        for (uint32_t i = 0; i < specifiedDims; ++i)
        {
            const value::Value& v = ctx->callArgs[i];
            int64_t size = 0;
            if (value::isInt(v))
                size = value::asInt(v);
            else
                throw errors::RuntimeException(
                    "JIT: jit_new_array_multi expected int64 dimension at slot " +
                    std::to_string(i));
            if (size < 0)
                throw errors::RuntimeException(
                    "Array dimension size cannot be negative: " + std::to_string(size));
            dimensions.push_back(size);
        }

        *dest = ctx->vm->createMultiArrayFromJit(typeIndex, dimensions,
                                                  static_cast<size_t>(totalDims));
    }

    void jit_array_get(value::Value* dest, const value::Value* array,
                        int64_t index)
    {
        if (value::isNativeArray(*array))
        {
            const auto& arr = value::asNativeArray(*array);
            *dest = arr->getUnchecked(index);
            return;
        }

        if (value::isFlatMultiArray(*array))
        {
            const auto& flatArray = value::asFlatMultiArray(*array);
            if (index < 0 || static_cast<size_t>(index) >= flatArray->size())
                jit_throw_array_oob(index, static_cast<int64_t>(flatArray->size()));
            if (flatArray->getRank() > 1)
                *dest = flatArray->getSubArray(static_cast<size_t>(index));
            else
                *dest = flatArray->get(static_cast<size_t>(index));
            return;
        }

        if (value::isSparseMultiArray(*array))
        {
            const auto& sparseArray = value::asSparseMultiArray(*array);
            if (index < 0 || static_cast<size_t>(index) >= sparseArray->size())
                jit_throw_array_oob(index, static_cast<int64_t>(sparseArray->size()));
            if (sparseArray->getRank() > 1)
            {
                *dest = sparseArray->getSubArray(static_cast<size_t>(index));
            }
            else
            {
                std::vector<size_t> indices = {static_cast<size_t>(index)};
                *dest = sparseArray->get(indices);
            }
            return;
        }

        throw errors::RuntimeException("ARRAY_GET on non-array value");
    }

    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue)
    {
        if (value::isNativeArray(*array))
        {
            const auto& arr = value::asNativeArray(*array);
            arr->setUnchecked(index, *newValue);
            return;
        }

        if (value::isFlatMultiArray(*array))
        {
            const auto& flatArray = value::asFlatMultiArray(*array);
            if (index < 0 || static_cast<size_t>(index) >= flatArray->size())
                jit_throw_array_oob(index, static_cast<int64_t>(flatArray->size()));
            flatArray->set(static_cast<size_t>(index), *newValue);
            return;
        }

        if (value::isSparseMultiArray(*array))
        {
            const auto& sparseArray = value::asSparseMultiArray(*array);
            if (index < 0 || static_cast<size_t>(index) >= sparseArray->size())
                jit_throw_array_oob(index, static_cast<int64_t>(sparseArray->size()));
            std::vector<size_t> indices = {static_cast<size_t>(index)};
            sparseArray->set(indices, *newValue);
            return;
        }

        throw errors::RuntimeException("ARRAY_SET on non-array value");
    }

    int64_t jit_array_length(const value::Value* array)
    {
        if (value::isNativeArray(*array))
        {
            const auto& arr = value::asNativeArray(*array);
            return static_cast<int64_t>(arr->size());
        }
        if (value::isFlatMultiArray(*array))
        {
            const auto& flatArray = value::asFlatMultiArray(*array);
            return static_cast<int64_t>(flatArray->size());
        }
        if (value::isSparseMultiArray(*array))
        {
            const auto& sparseArray = value::asSparseMultiArray(*array);
            return static_cast<int64_t>(sparseArray->size());
        }
        throw errors::RuntimeException("ARRAY_LENGTH on non-array value");
    }

    // Level 1: No Value construction, no atomic refcount
    int64_t jit_array_get_int(const value::Value* array, int64_t index)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_GET_INT on non-array value");
        const auto& arr = value::asNativeArray(*array);
        return arr->getIntDirect(static_cast<size_t>(index));
    }

    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_SET_INT on non-array value");
        const auto& arr = value::asNativeArray(*array);
        arr->setIntDirect(static_cast<size_t>(index), val);
    }

    // Level 2: Extract raw int64_t* data pointer for JIT inline access
    int64_t* jit_array_get_raw_int_ptr(const value::Value* array)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_GET_RAW_INT_PTR on non-array value");
        const auto& arr = value::asNativeArray(*array);
        return arr->getRawIntData();
    }

    void jit_array_extract_info(const value::Value* array, JitArrayInfo* out)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_EXTRACT_INFO on non-array value");
        const auto& arr = value::asNativeArray(*array);
        out->data = arr->getRawIntData();
        out->length = static_cast<int64_t>(arr->size());
    }

    void jit_array_get_field(value::Value* dest, const value::Value* array,
                             int64_t index,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_GET_FIELD on non-array value");
        auto arr = value::asNativeArray(*array);
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
            value::asObject(element);
        *dest = objInstance->getFieldValue(fieldName);
    }

    void jit_array_set_field(const value::Value* array, int64_t index,
                             const value::Value* newValue,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex)
    {
        if (!value::isNativeArray(*array))
            throw errors::RuntimeException("ARRAY_SET_FIELD on non-array value");
        auto arr = value::asNativeArray(*array);
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
            value::asObject(element);
        objInstance->setField(fieldName, *newValue);
    }
}
