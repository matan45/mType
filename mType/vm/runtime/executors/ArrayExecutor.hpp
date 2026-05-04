#pragma once
#include "../context/ExecutionContext.hpp"
#include <cstdint>
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ArrayPool.hpp"
#include "../../../value/FlatMultiArray.hpp"
#include "../../../value/SparseMultiArray.hpp"
#include <vector>

namespace environment::registry { class ClassRegistry; }

namespace vm::runtime
{
    /**
     * Executes array-related opcodes
     * Handles NEW_ARRAY, NEW_ARRAY_MULTI, ARRAY_GET, ARRAY_SET, ARRAY_LENGTH
     * Manages single and multi-dimensional array creation, access, and manipulation
     */
    class ArrayExecutor
    {
    public:
        explicit ArrayExecutor(ExecutionContext& ctx);
        ~ArrayExecutor() = default;

        // Array operations
        void handleNewArray(const bytecode::BytecodeProgram::Instruction& instr);
        void handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArrayGet();
        void handleArraySet();
        void handleArrayLength();

        // MYT-146: shared multi-dim factory usable from the interpreter executor
        // and from JIT runtime helpers. Takes pre-popped dimension sizes so it is
        // independent of any ExecutionContext.
        static value::Value buildMultiArray(
            environment::registry::ClassRegistry* classRegistry,
            const std::string& elementTypeName,
            const std::vector<int64_t>& dimensions,
            size_t totalDimensions);

        // SoA Field Access Optimization (avoids object materialization)
        void handleArrayGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArraySetField(const bytecode::BytecodeProgram::Instruction& instr);

        // Fused local-array operations (read array from local, skip stack copy)
        void handleArrayGetIntLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArraySetIntLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArrayLengthLocal(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Array creation helpers
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

        // ArrayPool-based creation for primitive multi-dimensional arrays
        value::Value createPooledMultiDimensionalArray(
            const std::vector<size_t>& dimensions,
            const std::string& elementTypeName
        );

        // Helper methods for handleArrayGet
        // Take shared_ptr by const-ref to avoid per-call refcount ops (MYT-200 trap).
        void getNativeArrayElement(const std::shared_ptr<value::NativeArray>& array, int64_t index);
        void getFlatMultiArrayElement(const std::shared_ptr<value::FlatMultiArray>& flatArray, int64_t index);
        void getSparseMultiArrayElement(const std::shared_ptr<value::SparseMultiArray>& sparseArray, int64_t index);

        // Helper methods for handleArraySet
        void setNativeArrayElement(const std::shared_ptr<value::NativeArray>& array, int64_t index, const value::Value& valueToSet);
        void setFlatMultiArrayElement(const std::shared_ptr<value::FlatMultiArray>& flatArray, int64_t index, const value::Value& valueToSet);
        void setSparseMultiArrayElement(const std::shared_ptr<value::SparseMultiArray>& sparseArray, int64_t index, const value::Value& valueToSet);
    };
}
