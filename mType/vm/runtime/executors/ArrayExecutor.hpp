#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ArrayPool.hpp"
#include "../../../value/FlatMultiArray.hpp"
#include "../../../value/SparseMultiArray.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/ArrayBoundsChecker.hpp"

namespace environment::registry { class ClassRegistry; }

namespace vm::runtime
{
    /**
     * Executes array-related opcodes
     * Handles NEW_ARRAY, NEW_ARRAY_MULTI, ARRAY_GET, ARRAY_SET, ARRAY_LENGTH
     * Manages single and multi-dimensional array creation, access, and manipulation
     *
     * MYT-319: hot dispatched handlers (GET / SET / LENGTH + their _LOCAL fused
     * forms, plus SoA field GET/SET) are inlined in the header so MSVC v145
     * (no /GL) can inline them through the dispatch switch. Array creation
     * (NEW_ARRAY / NEW_ARRAY_MULTI / buildMultiArray) and the heavy element
     * setter (setNativeArrayElement, with its full type-compat checking) stay
     * in the .cpp — they pull in ArrayFactory + ClassRegistry + InterfaceRegistry
     * and only fire once per array creation / object-array-write, not per
     * loop iteration.
     */
    class ArrayExecutor
    {
    public:
        explicit ArrayExecutor(ExecutionContext& ctx) : context(ctx) {}
        ~ArrayExecutor() = default;

        // Out-of-line: creation paths.
        void handleNewArray(const bytecode::BytecodeProgram::Instruction& instr);
        void handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr);

        inline void handleArrayGet() {
            // Pop index from stack
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);

            // Pop array from stack
            value::Value arrayVal = context.stackManager->pop();

            if (value::isNativeArray(arrayVal)) {
                getNativeArrayElement(value::asNativeArray(arrayVal), index, /*alias=*/false);
                return;
            }
            if (value::isFlatMultiArray(arrayVal)) {
                getFlatMultiArrayElement(value::asFlatMultiArray(arrayVal), index);
                return;
            }
            if (value::isSparseMultiArray(arrayVal)) {
                getSparseMultiArrayElement(value::asSparseMultiArray(arrayVal), index);
                return;
            }

            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context, "ARRAY_GET: Invalid array type");
        }

        // MYT-303: arr[i] result is about to be stored into a local (aliased).
        // Demotes SoA→heterogeneous so the local's reference identity is
        // shared with the array slot. Emitted by the peephole pass when an
        // ARRAY_GET is followed by STORE_LOCAL.
        inline void handleArrayGetAlias() {
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);

            value::Value arrayVal = context.stackManager->pop();

            if (value::isNativeArray(arrayVal)) {
                getNativeArrayElement(value::asNativeArray(arrayVal), index, /*alias=*/true);
                return;
            }
            if (value::isFlatMultiArray(arrayVal)) {
                getFlatMultiArrayElement(value::asFlatMultiArray(arrayVal), index);
                return;
            }
            if (value::isSparseMultiArray(arrayVal)) {
                getSparseMultiArrayElement(value::asSparseMultiArray(arrayVal), index);
                return;
            }

            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context, "ARRAY_GET_ALIAS: Invalid array type");
        }

        inline void handleArraySet() {
            value::Value valueToSet = context.stackManager->pop();
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);
            value::Value arrayVal = context.stackManager->pop();

            if (value::isNativeArray(arrayVal)) {
                setNativeArrayElement(value::asNativeArray(arrayVal), index, valueToSet);
                return;
            }
            if (value::isFlatMultiArray(arrayVal)) {
                setFlatMultiArrayElement(value::asFlatMultiArray(arrayVal), index, valueToSet);
                return;
            }
            if (value::isSparseMultiArray(arrayVal)) {
                setSparseMultiArrayElement(value::asSparseMultiArray(arrayVal), index, valueToSet);
                return;
            }

            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context, "ARRAY_SET: Invalid array type");
        }

        inline void handleArrayLength() {
            value::Value arrayVal = context.stackManager->pop();

            if (value::isNativeArray(arrayVal)) {
                const auto& array = value::asNativeArray(arrayVal);
                int64_t length = static_cast<int64_t>(array->size());
                context.stackManager->push(length);
                return;
            }
            if (value::isFlatMultiArray(arrayVal)) {
                const auto& flatArray = value::asFlatMultiArray(arrayVal);
                int64_t length = static_cast<int64_t>(flatArray->size());
                context.stackManager->push(length);
                return;
            }
            if (value::isSparseMultiArray(arrayVal)) {
                const auto& sparseArray = value::asSparseMultiArray(arrayVal);
                int64_t length = static_cast<int64_t>(sparseArray->size());
                context.stackManager->push(length);
                return;
            }

            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context, "ARRAY_LENGTH: Invalid array type");
        }

        // MYT-146: shared multi-dim factory usable from the interpreter executor
        // and from JIT runtime helpers. Takes pre-popped dimension sizes so it is
        // independent of any ExecutionContext.
        static value::Value buildMultiArray(
            environment::registry::ClassRegistry* classRegistry,
            const std::string& elementTypeName,
            const std::vector<int64_t>& dimensions,
            size_t totalDimensions);

        // SoA Field Access Optimization (avoids object materialization)
        inline void handleArrayGetField(const bytecode::BytecodeProgram::Instruction& instr) {
            // Get field name from constant pool
            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

            // Pop index from stack
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);

            // Pop array from stack
            value::Value arrayVal = context.stackManager->pop();
            auto array = value::asNativeArray(arrayVal);

            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

            size_t arrayIndex = static_cast<size_t>(index);

            // Check if this is a SoA ObjectArray (fast path)
            auto objectArray = array->getObjectArrayData();
            if (objectArray) {
                // FAST PATH: Direct field access from SoA structure.
                value::Value fieldValue = objectArray->getFieldUnchecked(arrayIndex, fieldName);
                context.stackManager->push(fieldValue);
                return;
            }

            // SLOW PATH: Array is not SoA-optimized, need to materialize object.
            value::Value element = array->getUnchecked(arrayIndex);

            if (value::isObject(element)) {
                auto objInstance = value::asObject(element);
                value::Value fieldValue = objInstance->getFieldValue(fieldName);
                context.stackManager->push(fieldValue);
            } else if (value::isValueObject(element)) {
                auto valueObj = value::asValueObject(element);
                value::Value fieldValue = valueObj->getFieldValue(fieldName);
                context.stackManager->push(fieldValue);
            } else {
                utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                    context,
                    "Cannot access field '" + fieldName + "' on non-object array element");
            }
        }

        inline void handleArraySetField(const bytecode::BytecodeProgram::Instruction& instr) {
            // Get field name from constant pool
            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

            value::Value valueToSet = context.stackManager->pop();
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);
            value::Value arrayVal = context.stackManager->pop();
            auto array = value::asNativeArray(arrayVal);

            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

            size_t arrayIndex = static_cast<size_t>(index);

            auto objectArray = array->getObjectArrayData();
            if (objectArray) {
                // FAST PATH: Direct field write to SoA structure.
                objectArray->setFieldUnchecked(arrayIndex, fieldName, valueToSet);
                return;
            }

            // SLOW PATH: materialize object.
            value::Value element = array->getUnchecked(arrayIndex);

            if (value::isObject(element)) {
                auto objInstance = value::asObject(element);
                objInstance->setField(fieldName, valueToSet);
            } else if (value::isValueObject(element)) {
                auto valueObj = value::asValueObject(element);
                valueObj->setField(fieldName, valueToSet);
                // Write back modified value object to the array (value semantics)
                array->setUnchecked(arrayIndex, value::Value(valueObj));
            } else {
                utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                    context,
                    "Cannot set field '" + fieldName + "' on non-object array element");
            }
        }

        // Fused local-array operations (read array from local, skip stack copy)
        inline void handleArrayGetIntLocal(const bytecode::BytecodeProgram::Instruction& instr) {
            size_t localSlot = instr.inlineOperands[0];
            size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
            const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);

            const auto& array = value::asNativeArray(arrayVal);
            context.stackManager->push(array->getIntDirect(static_cast<size_t>(index)));
        }

        inline void handleArraySetIntLocal(const bytecode::BytecodeProgram::Instruction& instr) {
            size_t localSlot = instr.inlineOperands[0];
            size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
            const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

            value::Value valToSet = context.stackManager->pop();
            value::Value indexVal = context.stackManager->pop();
            int64_t index = value::asInt(indexVal);
            int64_t val = value::asInt(valToSet);

            const auto& array = value::asNativeArray(arrayVal);
            array->setIntDirect(static_cast<size_t>(index), val);
        }

        inline void handleArrayLengthLocal(const bytecode::BytecodeProgram::Instruction& instr) {
            size_t localSlot = instr.inlineOperands[0];
            size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
            const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

            // Match handleArrayLength's dispatch.
            if (value::isNativeArray(arrayVal)) {
                const auto& array = value::asNativeArray(arrayVal);
                context.stackManager->push(static_cast<int64_t>(array->size()));
                return;
            }
            if (value::isFlatMultiArray(arrayVal)) {
                const auto& array = value::asFlatMultiArray(arrayVal);
                context.stackManager->push(static_cast<int64_t>(array->size()));
                return;
            }
            if (value::isSparseMultiArray(arrayVal)) {
                const auto& array = value::asSparseMultiArray(arrayVal);
                context.stackManager->push(static_cast<int64_t>(array->size()));
                return;
            }
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context, "ARRAY_LENGTH_LOCAL: Invalid array type");
        }

    private:
        ExecutionContext& context;

        // Array creation helpers (out-of-line).
        std::shared_ptr<value::NativeArray> createJaggedArray(
            const std::vector<int64_t>& dimensions,
            size_t dimIndex,
            const std::string& elementTypeName,
            size_t totalDimensions
        );

        std::shared_ptr<value::NativeArray> createNestedArray(
            const std::vector<int64_t>& dimensions,
            size_t dimIndex,
            const std::string& elementTypeName
        );

        value::Value createPooledMultiDimensionalArray(
            const std::vector<size_t>& dimensions,
            const std::string& elementTypeName
        );

        // Inlined helpers for the hot GET/SET dispatch paths.
        inline void getNativeArrayElement(const std::shared_ptr<value::NativeArray>& array, int64_t index, bool alias) {
            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

            // MYT-303: a full object element load only escapes into a named local
            // when the consumer stores it. For transient consumers we keep SoA
            // storage and let materializeInstance hand out a snapshot.
            if (alias && array->getObjectArrayData()) {
                array->materializeObjectStorageForAlias();
            }

            // Get element using unchecked access (bounds already verified).
            value::Value element = array->getUnchecked(static_cast<size_t>(index));

            // Deep copy ValueObjects when retrieving from arrays (value semantics).
            // Gate the isValueObject check on element-type == OBJECT.
            if (array->getElementType() == value::ValueType::OBJECT && value::isValueObject(element)) {
                auto valueObj = value::asValueObject(element);
                element = value::Value(valueObj->deepCopy());
            }

            context.stackManager->push(std::move(element));
        }

        inline void getFlatMultiArrayElement(const std::shared_ptr<value::FlatMultiArray>& flatArray, int64_t index) {
            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), flatArray->size(), "FlatMultiArray");

            // For multi-dimensional arrays, return sub-array; for 1D, return element.
            if (flatArray->getRank() > 1) {
                context.stackManager->push(value::Value(flatArray->getSubArray(static_cast<size_t>(index))));
            } else {
                context.stackManager->push(flatArray->get(static_cast<size_t>(index)));
            }
        }

        inline void getSparseMultiArrayElement(const std::shared_ptr<value::SparseMultiArray>& sparseArray, int64_t index) {
            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), sparseArray->size(), "SparseMultiArray");

            if (sparseArray->getRank() > 1) {
                context.stackManager->push(value::Value(sparseArray->getSubArray(static_cast<size_t>(index))));
            } else {
                std::vector<size_t> indices = {static_cast<size_t>(index)};
                context.stackManager->push(sparseArray->get(indices));
            }
        }

        // Out-of-line: setNativeArrayElement is heavy (~150L type-compat checking
        // pulls in ClassRegistry/InterfaceRegistry — keep in .cpp).
        void setNativeArrayElement(const std::shared_ptr<value::NativeArray>& array, int64_t index, const value::Value& valueToSet);

        inline void setFlatMultiArrayElement(const std::shared_ptr<value::FlatMultiArray>& flatArray, int64_t index, const value::Value& valueToSet) {
            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), flatArray->size(), "FlatMultiArray");

            if (flatArray->getRank() == 1) {
                flatArray->set(static_cast<size_t>(index), valueToSet);
            } else {
                utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                    context,
                    "Cannot set element in multi-dimensional FlatMultiArray with single index");
            }
        }

        inline void setSparseMultiArrayElement(const std::shared_ptr<value::SparseMultiArray>& sparseArray, int64_t index, const value::Value& valueToSet) {
            utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), sparseArray->size(), "SparseMultiArray");

            if (sparseArray->getRank() == 1) {
                std::vector<size_t> indices = {static_cast<size_t>(index)};
                sparseArray->set(indices, valueToSet);
            } else {
                utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                    context,
                    "Cannot set element in multi-dimensional SparseMultiArray with single index");
            }
        }
    };
}
