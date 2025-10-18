#pragma once
#include "../context/ExecutionContext.hpp"
#include "../utils/TypeConverter.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ArrayPool.hpp"
#include "../../../value/FlatMultiArray.hpp"
#include "../../../value/SparseMultiArray.hpp"
#include <vector>

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

        // SoA Field Access Optimization (avoids object materialization)
        void handleArrayGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArraySetField(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Array creation helpers
        std::shared_ptr<value::NativeArray> createJaggedArray(
            const std::vector<int>& dimensions,
            size_t dimIndex,
            const std::string& elementTypeName,
            size_t totalDimensions
        );

        std::shared_ptr<value::NativeArray> createNestedArray(
            const std::vector<int>& dimensions,
            size_t dimIndex,
            const std::string& elementTypeName
        );

        // ArrayPool-based creation for primitive multi-dimensional arrays
        value::Value createPooledMultiDimensionalArray(
            const std::vector<size_t>& dimensions,
            const std::string& elementTypeName
        );
    };
}
