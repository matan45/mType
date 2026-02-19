#include "JitHelpers.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include "../../value/NativeArray.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::jit
{
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
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        *dest = arr->getUnchecked(index);
    }

    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue)
    {
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        arr->setUnchecked(index, *newValue);
    }

    int64_t jit_array_length(const value::Value* array)
    {
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return static_cast<int64_t>(arr->size());
    }

    // Level 1: No Value construction, no atomic refcount
    int64_t jit_array_get_int(const value::Value* array, int64_t index)
    {
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return arr->getIntDirect(static_cast<size_t>(index));
    }

    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val)
    {
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        arr->setIntDirect(static_cast<size_t>(index), val);
    }

    // Level 2: Extract raw int64_t* data pointer for JIT inline access
    int64_t* jit_array_get_raw_int_ptr(const value::Value* array)
    {
        const auto& arr = std::get<std::shared_ptr<value::NativeArray>>(*array);
        return arr->getRawIntData();
    }

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
