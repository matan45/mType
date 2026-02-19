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
