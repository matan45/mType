#include "ArrayExecutor.hpp"
#include "../../../value/arrays/ArrayFactory.hpp"
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

        value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
        // Use ArrayFactory for optimized array creation
        // ClassRegistry will be looked up by ArrayFactory if available
        auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;
        auto array = mType::value::arrays::ArrayFactory::create1DArray(
            static_cast<size_t>(size),
             elemType,
            elementTypeName,
            nullptr,  // ClassDefinition will be resolved by ArrayFactory
            classRegistry  // Pass ClassRegistry for lookup
        );

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

        // Note: Multi-dimensional arrays currently use nested NativeArray approach
        // FlatMultiObjectArray with full SoA optimization is not yet integrated with bytecode VM
        // because ARRAY_GET/ARRAY_SET bytecode operations expect NativeArray types
        //
        // To get SoA benefits in multi-dimensional arrays, the innermost dimension should be >= 16
        // Example: Person[4][20] will get SoA for the inner arrays

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

        // Bounds check (VM does bounds check once)
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()), errorLoc);
            } else {
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()));
            }
        }

        // Get element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->get()
        value::Value element = array->getUnchecked(static_cast<size_t>(index));
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

        // Bounds check (VM does bounds check once)
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()), errorLoc);
            } else {
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()));
            }
        }

        // Set element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->set()
        array->setUnchecked(static_cast<size_t>(index), valueToSet);
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
            auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;

            if (dimensions.size() == totalDimensions) {
                // Fully specified - create array of elements using ArrayFactory
                value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
                return mType::value::arrays::ArrayFactory::create1DArray(
                    currentDimSize,
                    elemType,
                    elementTypeName,
                    nullptr,
                    classRegistry
                );
            } else {
                // Jagged - create array of null array references
                auto array = mType::value::arrays::ArrayFactory::create1DArray(
                    currentDimSize,
                    value::ValueType::OBJECT,
                    "Array",
                    nullptr,
                    classRegistry
                );
                // Elements are initialized to std::monostate{} (null) by default
                return array;
            }
        }
        else {
            // Not last dimension - create array of arrays using ArrayFactory
            auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;
            auto outerArray = mType::value::arrays::ArrayFactory::create1DArray(
                currentDimSize,
                value::ValueType::OBJECT,
                "Array",
                nullptr,
                classRegistry
            );

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
        auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;

        if (dimIndex == dimensions.size() - 1) {
            // Last dimension - create array of actual elements using ArrayFactory
            value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
            return mType::value::arrays::ArrayFactory::create1DArray(
                currentDimSize,
                elemType,
                elementTypeName,
                nullptr,
                classRegistry
            );
        }
        else {
            // Not last dimension - create array of arrays using ArrayFactory
            auto outerArray = mType::value::arrays::ArrayFactory::create1DArray(
                currentDimSize,
                value::ValueType::OBJECT,
                "Array",
                nullptr,
                classRegistry
            );

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i) {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    void ArrayExecutor::handleArrayGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get field name from constant pool
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check (VM does bounds check once)
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()), errorLoc);
            } else {
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()));
            }
        }

        size_t arrayIndex = static_cast<size_t>(index);

        // Check if this is a SoA ObjectArray (fast path)
        auto objectArray = array->getObjectArrayData();
        if (objectArray) {
            // FAST PATH: Direct field access from SoA structure!
            // PERFORMANCE: Avoids expensive object materialization (~200 ns → ~8-10 ns)
            value::Value fieldValue = objectArray->getFieldUnchecked(arrayIndex, fieldName);
            context.stackManager->push(fieldValue);
            return;
        }

        // SLOW PATH: Array is not SoA-optimized, need to materialize object
        // This happens for:
        // - Small object arrays (< 16 elements)
        // - Heterogeneous arrays
        // - Arrays without ClassDefinition
        value::Value element = array->getUnchecked(arrayIndex);

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element)) {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element);
            value::Value fieldValue = objInstance->getFieldValue(fieldName);
            context.stackManager->push(fieldValue);
        } else {
            throw errors::RuntimeException("Cannot access field '" + fieldName +
                                         "' on non-object array element");
        }
    }

    void ArrayExecutor::handleArraySetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get field name from constant pool
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop value to set from stack
        value::Value valueToSet = context.stackManager->pop();

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check (VM does bounds check once)
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()), errorLoc);
            } else {
                throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                             " for array of size " + std::to_string(array->size()));
            }
        }

        size_t arrayIndex = static_cast<size_t>(index);

        // Check if this is a SoA ObjectArray (fast path)
        auto objectArray = array->getObjectArrayData();
        if (objectArray) {
            // FAST PATH: Direct field write to SoA structure!
            // PERFORMANCE: Avoids expensive object materialization (~200 ns → ~8-10 ns)
            objectArray->setFieldUnchecked(arrayIndex, fieldName, valueToSet);
            return;
        }

        // SLOW PATH: Array is not SoA-optimized, need to materialize object
        value::Value element = array->getUnchecked(arrayIndex);

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element)) {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(element);
            objInstance->setField(fieldName, valueToSet);
        } else {
            throw errors::RuntimeException("Cannot set field '" + fieldName +
                                         "' on non-object array element");
        }
    }
}
