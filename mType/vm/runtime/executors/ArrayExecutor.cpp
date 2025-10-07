#include "ArrayExecutor.hpp"
#include <algorithm>

namespace vm::runtime
{
    ArrayExecutor::ArrayExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void ArrayExecutor::handleNewArray(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop array size from stack
        value::Value sizeVal = context.stackManager->pop();
        int size = std::get<int>(sizeVal);

        if (size < 0) {
            throw errors::RuntimeException("Array size cannot be negative: " + std::to_string(size));
        }

        // Create new NativeArray
        auto array = std::make_shared<value::NativeArray>(size, value::ValueType::OBJECT, elementTypeName);

        // Push array onto stack
        context.stackManager->push(array);
    }

    void ArrayExecutor::handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = context.program->getConstantPool().getString(instr.operands[0]);
        // Compiler emits: [typeIndex, dimensionCount (total), specifiedDimensions]
        size_t totalDimensions = instr.operands[1];  // Total number of dimensions
        size_t specifiedDimensions = instr.operands.size() > 2 ? instr.operands[2] : totalDimensions;

        // Pop dimension sizes from stack (in reverse order - last dimension first)
        // Only pop the number of specified dimensions
        std::vector<int> dimensions;
        dimensions.reserve(specifiedDimensions);
        for (size_t i = 0; i < specifiedDimensions; ++i) {
            value::Value sizeVal = context.stackManager->pop();
            int size = std::get<int>(sizeVal);
            if (size < 0) {
                throw errors::RuntimeException("Array dimension size cannot be negative: " + std::to_string(size));
            }
            dimensions.push_back(size);
        }
        std::reverse(dimensions.begin(), dimensions.end());

        // For jagged arrays (totalDimensions > specifiedDimensions), we only create
        // the specified outer dimensions. Inner dimensions remain as null references.
        // For fully specified arrays, create nested arrays for all dimensions.

        // Create the array structure based on specified dimensions
        std::shared_ptr<value::NativeArray> result = createJaggedArray(dimensions, 0, elementTypeName, totalDimensions);

        context.stackManager->push(result);
    }

    void ArrayExecutor::handleArrayGet() {
        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                         " for array of size " + std::to_string(array->size()));
        }

        // Get element and push onto stack
        value::Value element = array->get(index);
        context.stackManager->push(element);
    }

    void ArrayExecutor::handleArraySet() {
        // Pop value from stack
        value::Value valueToSet = context.stackManager->pop();

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                         " for array of size " + std::to_string(array->size()));
        }

        // Set element
        array->set(index, valueToSet);
    }

    void ArrayExecutor::handleArrayLength() {
        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Get length and push as integer onto stack
        int length = static_cast<int>(array->size());
        context.stackManager->push(length);
    }

    std::shared_ptr<value::NativeArray> ArrayExecutor::createJaggedArray(
        const std::vector<int>& dimensions,
        size_t dimIndex,
        const std::string& elementTypeName,
        size_t totalDimensions)
    {
        if (dimIndex >= dimensions.size()) {
            throw errors::RuntimeException("Invalid dimension index in jagged array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1) {
            // Last specified dimension
            // If this is also the last total dimension, create array of actual elements
            // Otherwise, create array of null references (for jagged arrays)
            if (dimensions.size() == totalDimensions) {
                // Fully specified - create array of elements
                // Convert element type name to ValueType
                value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
                return std::make_shared<value::NativeArray>(currentDimSize, elemType, elementTypeName);
            } else {
                // Jagged - create array of null array references
                auto array = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");
                // Elements are initialized to std::monostate{} (null) by default
                return array;
            }
        }
        else {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i) {
                auto innerArray = createJaggedArray(dimensions, dimIndex + 1, elementTypeName, totalDimensions);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    std::shared_ptr<value::NativeArray> ArrayExecutor::createNestedArray(
        const std::vector<int>& dimensions,
        size_t dimIndex,
        const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size()) {
            throw errors::RuntimeException("Invalid dimension index in multi-dimensional array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1) {
            // Last dimension - create array of actual elements
            return std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, elementTypeName);
        }
        else {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i) {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }
}
