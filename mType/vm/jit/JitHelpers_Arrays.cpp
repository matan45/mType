#include "JitHelpers.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include "../../value/NativeArray.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../bytecode/BytecodeProgram.hpp"

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

    void jit_array_get(value::Value* dest, const value::Value* array,
                        int64_t index)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_GET on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        *dest = arr->getUnchecked(index);
    }

    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_SET on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        arr->setUnchecked(index, *newValue);
    }

    int64_t jit_array_length(const value::Value* array)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_LENGTH on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return static_cast<int64_t>(arr->size());
    }

    // Level 1: No Value construction, no atomic refcount
    int64_t jit_array_get_int(const value::Value* array, int64_t index)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_GET_INT on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return arr->getIntDirect(static_cast<size_t>(index));
    }

    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_SET_INT on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        arr->setIntDirect(static_cast<size_t>(index), val);
    }

    // Level 2: Extract raw int64_t* data pointer for JIT inline access
    int64_t* jit_array_get_raw_int_ptr(const value::Value* array)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_GET_RAW_INT_PTR on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return arr->getRawIntData();
    }

    void jit_array_extract_info(const value::Value* array, JitArrayInfo* out)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_EXTRACT_INFO on non-array value");
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        out->data = arr->getRawIntData();
        out->length = static_cast<int64_t>(arr->size());
    }

    void jit_array_get_field(value::Value* dest, const value::Value* array,
                             int64_t index,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex)
    {
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_GET_FIELD on non-array value");
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
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(*array))
            throw errors::RuntimeException("ARRAY_SET_FIELD on non-array value");
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
