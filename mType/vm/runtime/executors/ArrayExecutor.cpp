#include "ArrayExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/ArrayBoundsChecker.hpp"
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

        // Use ArrayPool for fully-specified primitive multi-dimensional arrays
        // Both FlatMultiArray and SparseMultiArray support view-based sub-arrays!
        // Modifications to sub-arrays propagate to the parent array.
        // ArrayPool adaptively chooses between dense (FlatMultiArray) and sparse (SparseMultiArray) storage.
        value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
        bool isPrimitive = (elemType == value::ValueType::INT ||
                           elemType == value::ValueType::FLOAT ||
                           elemType == value::ValueType::BOOL ||
                           elemType == value::ValueType::STRING);

        if (dimensions.size() == totalDimensions && dimensions.size() > 1 && isPrimitive) {
            // Use ArrayPool with adaptive allocation (automatically chooses optimal storage mode)
            std::vector<size_t> sizeDimensions(dimensions.begin(), dimensions.end());
            value::Value pooledArray = createPooledMultiDimensionalArray(sizeDimensions, elementTypeName);
            context.stackManager->push(pooledArray);
            return;
        }

        // Note: For object arrays or jagged arrays, use nested NativeArray approach
        // FlatMultiObjectArray with full SoA optimization is not yet integrated with bytecode VM
        // because ARRAY_GET/ARRAY_SET bytecode operations expect NativeArray types
        //
        // To get SoA benefits in multi-dimensional arrays, the innermost dimension should be >= 16
        // Example: Person[4][20] will get SoA for the inner arrays

        // Create the array structure based on specified dimensions
        std::shared_ptr<value::NativeArray> result = createJaggedArray(dimensions, 0, elementTypeName, totalDimensions);

        context.stackManager->push(result);
    }

    void ArrayExecutor::getNativeArrayElement(std::shared_ptr<value::NativeArray> array, int index) {
        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, index, array->size(), "Array");

        // Get element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->get()
        value::Value element = array->getUnchecked(static_cast<size_t>(index));
        context.stackManager->push(element);
    }

    void ArrayExecutor::getFlatMultiArrayElement(std::shared_ptr<value::FlatMultiArray> flatArray, int index) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, index, flatArray->size(), "FlatMultiArray");

        // For multi-dimensional arrays, return sub-array; for 1D, return element
        if (flatArray->getRank() > 1) {
            auto subArray = flatArray->getSubArray(static_cast<size_t>(index));
            context.stackManager->push(subArray);
        } else {
            value::Value element = flatArray->get(static_cast<size_t>(index));
            context.stackManager->push(element);
        }
    }

    void ArrayExecutor::getSparseMultiArrayElement(std::shared_ptr<value::SparseMultiArray> sparseArray, int index) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, index, sparseArray->size(), "SparseMultiArray");

        // For multi-dimensional arrays, return sub-array; for 1D, return element
        if (sparseArray->getRank() > 1) {
            auto subArray = sparseArray->getSubArray(static_cast<size_t>(index));
            context.stackManager->push(subArray);
        } else {
            std::vector<size_t> indices = {static_cast<size_t>(index)};
            value::Value element = sparseArray->get(indices);
            context.stackManager->push(element);
        }
    }

    void ArrayExecutor::handleArrayGet() {
        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();

        // Handle NativeArray (1D arrays and nested multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayVal)) {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
            getNativeArrayElement(array, index);
            return;
        }

        // Handle FlatMultiArray (pooled dense multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(arrayVal)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(arrayVal);
            getFlatMultiArrayElement(flatArray, index);
            return;
        }

        // Handle SparseMultiArray (pooled sparse multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(arrayVal)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(arrayVal);
            getSparseMultiArrayElement(sparseArray, index);
            return;
        }

        throw errors::RuntimeException("ARRAY_GET: Invalid array type");
    }

    void ArrayExecutor::setNativeArrayElement(std::shared_ptr<value::NativeArray> array, int index, const value::Value& valueToSet) {
        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, index, array->size(), "Array");

        // Set element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->set()
        array->setUnchecked(static_cast<size_t>(index), valueToSet);
    }

    void ArrayExecutor::setFlatMultiArrayElement(std::shared_ptr<value::FlatMultiArray> flatArray, int index, const value::Value& valueToSet) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, index, flatArray->size(), "FlatMultiArray");

        // For 1D arrays, set directly; multi-dimensional arrays cannot be set this way
        if (flatArray->getRank() == 1) {
            flatArray->set(static_cast<size_t>(index), valueToSet);
        } else {
            throw errors::RuntimeException("Cannot set element in multi-dimensional FlatMultiArray with single index");
        }
    }

    void ArrayExecutor::setSparseMultiArrayElement(std::shared_ptr<value::SparseMultiArray> sparseArray, int index, const value::Value& valueToSet) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, index, sparseArray->size(), "SparseMultiArray");

        // For 1D arrays, set directly; multi-dimensional arrays cannot be set this way
        if (sparseArray->getRank() == 1) {
            std::vector<size_t> indices = {static_cast<size_t>(index)};
            sparseArray->set(indices, valueToSet);
        } else {
            throw errors::RuntimeException("Cannot set element in multi-dimensional SparseMultiArray with single index");
        }
    }

    void ArrayExecutor::handleArraySet() {
        // Pop value from stack
        value::Value valueToSet = context.stackManager->pop();

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();

        // Handle NativeArray (1D arrays and nested multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayVal)) {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
            setNativeArrayElement(array, index, valueToSet);
            return;
        }

        // Handle FlatMultiArray (pooled dense multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(arrayVal)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(arrayVal);
            setFlatMultiArrayElement(flatArray, index, valueToSet);
            return;
        }

        // Handle SparseMultiArray (pooled sparse multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(arrayVal)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(arrayVal);
            setSparseMultiArrayElement(sparseArray, index, valueToSet);
            return;
        }

        throw errors::RuntimeException("ARRAY_SET: Invalid array type");
    }

    void ArrayExecutor::handleArrayLength() {
        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();

        // Handle NativeArray
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayVal)) {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
            int length = static_cast<int>(array->size());
            context.stackManager->push(length);
            return;
        }

        // Handle FlatMultiArray
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(arrayVal)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(arrayVal);
            int length = static_cast<int>(flatArray->size());
            context.stackManager->push(length);
            return;
        }

        // Handle SparseMultiArray
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(arrayVal)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(arrayVal);
            int length = static_cast<int>(sparseArray->size());
            context.stackManager->push(length);
            return;
        }

        throw errors::RuntimeException("ARRAY_LENGTH: Invalid array type");
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
        utils::ArrayBoundsChecker::checkBounds(context, index, array->size(), "Array");

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
        utils::ArrayBoundsChecker::checkBounds(context, index, array->size(), "Array");

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

    value::Value ArrayExecutor::createPooledMultiDimensionalArray(
        const std::vector<size_t>& dimensions,
        const std::string& elementTypeName)
    {
        // Get default value for the element type
        value::ValueType elemType = utils::TypeConverter::stringToValueType(elementTypeName);
        value::Value defaultValue;

        switch (elemType) {
            case value::ValueType::INT:
                defaultValue = 0;
                break;
            case value::ValueType::FLOAT:
                defaultValue = 0.0f;
                break;
            case value::ValueType::BOOL:
                defaultValue = false;
                break;
            case value::ValueType::STRING:
                defaultValue = std::string("");
                break;
            default:
                defaultValue = std::monostate{}; // null for objects
                break;
        }

        // Use ArrayPool for adaptive multi-dimensional array allocation
        // PERFORMANCE: ArrayPool provides:
        // - Pooling for common dimension patterns (reduces allocation overhead)
        // - Adaptive selection between FlatMultiArray (dense) and SparseMultiArray (sparse)
        // - Better cache locality for pooled arrays
        // - View-based sub-arrays (modifications propagate to parent!)
        //
        // Both FlatMultiArray and SparseMultiArray now support view-based sub-arrays!
        auto& pool = value::ArrayPool::getInstance();
        return pool.acquireAdaptive(dimensions, defaultValue);
    }
}
