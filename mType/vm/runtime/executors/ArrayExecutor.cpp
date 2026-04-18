#include "ArrayExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/ArrayBoundsChecker.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../environment/registry/ClassRegistry.hpp"
#include "../../../value/arrays/ArrayFactory.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include <algorithm>
#include <functional>

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
        int64_t size = std::get<int64_t>(sizeVal);

        if (size < 0) {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Array size cannot be negative: " + std::to_string(size)
            );
        }

        value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
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
        std::vector<int64_t> dimensions;
        dimensions.reserve(specifiedDimensions);
        for (size_t i = 0; i < specifiedDimensions; ++i) {
            value::Value sizeVal = context.stackManager->pop();
            int64_t size = std::get<int64_t>(sizeVal);
            if (size < 0) {
                utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                    context,
                    "Array dimension size cannot be negative: " + std::to_string(size)
                );
            }
            dimensions.push_back(size);
        }
        std::reverse(dimensions.begin(), dimensions.end());

        auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;
        value::Value result = buildMultiArray(classRegistry, elementTypeName, dimensions, totalDimensions);
        context.stackManager->push(result);
    }

    // MYT-146: pure-factory entry point - used by both handleNewArrayMulti (above)
    // and VirtualMachine::createMultiArrayFromJit. Throws plain RuntimeException on
    // invalid state (no source-location decoration since JIT helpers don't have
    // an ExecutionContext to thread through).
    value::Value ArrayExecutor::buildMultiArray(
        environment::registry::ClassRegistry* classRegistry,
        const std::string& elementTypeName,
        const std::vector<int64_t>& dimensions,
        size_t totalDimensions)
    {
        value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
        bool isPrimitive = (elemType == value::ValueType::INT ||
                            elemType == value::ValueType::FLOAT ||
                            elemType == value::ValueType::BOOL ||
                            elemType == value::ValueType::STRING);

        // Pooled primitive multi-dim fast path (FlatMultiArray / SparseMultiArray via
        // ArrayPool::acquireAdaptive). Both support view-based sub-arrays, so
        // mutations through sub-arrays propagate to the parent.
        if (dimensions.size() == totalDimensions && dimensions.size() > 1 && isPrimitive) {
            value::Value defaultValue;
            switch (elemType) {
                case value::ValueType::INT:    defaultValue = static_cast<int64_t>(0); break;
                case value::ValueType::FLOAT:  defaultValue = 0.0; break;
                case value::ValueType::BOOL:   defaultValue = false; break;
                case value::ValueType::STRING: defaultValue = std::string(""); break;
                default:                        defaultValue = nullptr; break;
            }
            std::vector<size_t> sizeDimensions(dimensions.begin(), dimensions.end());
            return value::ArrayPool::getInstance().acquireAdaptive(sizeDimensions, defaultValue);
        }

        // Jagged / object path - nested NativeArray structure.
        std::function<std::shared_ptr<value::NativeArray>(size_t)> makeJagged =
            [&](size_t dimIndex) -> std::shared_ptr<value::NativeArray> {
                if (dimIndex >= dimensions.size()) {
                    throw errors::RuntimeException(
                        "Invalid dimension index in jagged array creation");
                }
                int64_t currentDimSize = dimensions[dimIndex];
                if (dimIndex == dimensions.size() - 1) {
                    if (dimensions.size() == totalDimensions) {
                        return mType::value::arrays::ArrayFactory::create1DArray(
                            currentDimSize, elemType, elementTypeName, nullptr, classRegistry);
                    }
                    return mType::value::arrays::ArrayFactory::create1DArray(
                        currentDimSize, value::ValueType::OBJECT, "Array", nullptr, classRegistry);
                }
                auto outerArray = mType::value::arrays::ArrayFactory::create1DArray(
                    currentDimSize, value::ValueType::OBJECT, "Array", nullptr, classRegistry);
                for (int i = 0; i < currentDimSize; ++i) {
                    outerArray->set(i, makeJagged(dimIndex + 1));
                }
                return outerArray;
            };
        return makeJagged(0);
    }

    void ArrayExecutor::getNativeArrayElement(std::shared_ptr<value::NativeArray> array, int64_t index) {
        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

        // Get element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->get()
        value::Value element = array->getUnchecked(static_cast<size_t>(index));

        // Deep copy ValueObjects when retrieving from arrays (value semantics)
        // This ensures list.get(0).x = 5 doesn't modify the value inside the collection
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(element)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(element);
            element = value::Value(valueObj->deepCopy());
        }

        context.stackManager->push(element);
    }

    void ArrayExecutor::getFlatMultiArrayElement(std::shared_ptr<value::FlatMultiArray> flatArray, int64_t index) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), flatArray->size(), "FlatMultiArray");

        // For multi-dimensional arrays, return sub-array; for 1D, return element
        if (flatArray->getRank() > 1) {
            auto subArray = flatArray->getSubArray(static_cast<size_t>(index));
            context.stackManager->push(subArray);
        } else {
            value::Value element = flatArray->get(static_cast<size_t>(index));
            context.stackManager->push(element);
        }
    }

    void ArrayExecutor::getSparseMultiArrayElement(std::shared_ptr<value::SparseMultiArray> sparseArray, int64_t index) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), sparseArray->size(), "SparseMultiArray");

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
        int64_t index = std::get<int64_t>(indexVal);

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

        utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
            context,
            "ARRAY_GET: Invalid array type"
        );
    }

    void ArrayExecutor::setNativeArrayElement(std::shared_ptr<value::NativeArray> array, int64_t index, const value::Value& valueToSet) {
        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

        // Type check for object arrays with generics (e.g., Box<Int>[] cannot accept Box<String>)
        if (array->getElementType() == value::ValueType::OBJECT) {
            std::string expectedTypeName = array->getElementTypeName();

            // Check for array-to-array assignment (e.g., int[][] where string[] is assigned to int[] slot)
            if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(valueToSet)) {
                auto valueArray = std::get<std::shared_ptr<value::NativeArray>>(valueToSet);
                std::string actualArrayElementType = valueArray->getElementTypeName();
                value::ValueType actualElementValueType = valueArray->getElementType();

                // Check if there's already an element at this index to compare types
                // This validates that all sub-arrays have consistent element types
                if (index < static_cast<int>(array->size())) {
                    value::Value existingElement = array->get(static_cast<size_t>(index));

                    // If there's an existing non-null array, compare element types
                    if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(existingElement)) {
                        auto existingArray = std::get<std::shared_ptr<value::NativeArray>>(existingElement);
                        std::string existingElementType = existingArray->getElementTypeName();
                        value::ValueType existingElementValueType = existingArray->getElementType();

                        // For primitive types (int, float, string, bool), enforce strict type matching
                        // For object types, allow polymorphism (subclass can be assigned to parent type)
                        bool isPrimitiveType = (actualElementValueType == value::ValueType::INT ||
                                              actualElementValueType == value::ValueType::FLOAT ||
                                              actualElementValueType == value::ValueType::STRING ||
                                              actualElementValueType == value::ValueType::BOOL);

                        if (isPrimitiveType) {
                            // Strict type check for primitives
                            if (existingElementType != actualArrayElementType) {
                                utils::ErrorLocationHelper::throwError<errors::TypeException>(
                                    context,
                                    "Array element type mismatch: cannot assign " + actualArrayElementType +
                                    "[] to slot expecting " + existingElementType + "[]"
                                );
                            }
                        } else if (actualElementValueType == value::ValueType::OBJECT &&
                                   existingElementValueType == value::ValueType::OBJECT) {
                            // For object types, check polymorphism (is actualType a subclass/implementation of existingType?)
                            // If types are identical, allow it
                            if (existingElementType != actualArrayElementType) {
                                // Check if actualType is compatible with existingType via:
                                // 1. Inheritance (actualClass extends existingType)
                                // 2. Interface implementation (actualClass implements existingType)
                                bool isCompatible = false;

                                if (context.environment) {
                                    auto classRegistry = context.environment->getClassRegistry();
                                    auto actualClass = classRegistry->findClass(actualArrayElementType);

                                    if (actualClass) {
                                        // Check inheritance: does actualClass extend existingElementType?
                                        auto currentClass = actualClass;
                                        while (currentClass && currentClass->hasParentClass()) {
                                            if (currentClass->getParentClassName() == existingElementType) {
                                                isCompatible = true;
                                                break;
                                            }
                                            currentClass = classRegistry->findClass(currentClass->getParentClassName());
                                        }

                                        // If not compatible via inheritance, check interface implementation
                                        if (!isCompatible) {
                                            // Check if existingElementType is an interface
                                            auto interfaceRegistry = context.environment->getInterfaceRegistry();
                                            auto interfaceDef = interfaceRegistry->findInterface(existingElementType);

                                            if (interfaceDef) {
                                                // existingElementType is an interface
                                                // Check if actualClass implements this interface
                                                const auto& implementedInterfaces = actualClass->getImplementedInterfaces();
                                                for (const auto& implInterface : implementedInterfaces) {
                                                    if (implInterface == existingElementType) {
                                                        isCompatible = true;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                if (!isCompatible) {
                                    utils::ErrorLocationHelper::throwError<errors::TypeException>(
                                        context,
                                        "Array element type mismatch: cannot assign " + actualArrayElementType +
                                        "[] to slot expecting " + existingElementType + "[] (not compatible types)"
                                    );
                                }
                            }
                        }
                    }
                }
            }

            // Only enforce strict type checking for generic types with type arguments (contains '<' and '>')
            // This prevents Box<String> being assigned to Box<Int>[] while allowing:
            // - Generic type parameters like T[] to accept any type
            // - Normal polymorphism like Person to Object[]
            bool isGenericInstantiation = (expectedTypeName.find('<') != std::string::npos);

            // Check if value is an object instance
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(valueToSet)) {
                auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(valueToSet);

                // Get the full type name including generic type arguments
                std::string actualTypeName = objInstance->getFullTypeName();

                // For generic instantiations, we now compare the full type names
                if (isGenericInstantiation) {
                    // Compare full generic type names (e.g., "Box<Int>" vs "Box<String>")
                    if (expectedTypeName != actualTypeName) {
                        utils::ErrorLocationHelper::throwError<errors::TypeException>(
                            context,
                            "Array element type mismatch: cannot assign " + actualTypeName +
                            " to array of type " + expectedTypeName + "[]"
                        );
                    }
                }
            }
            // Allow null assignment to object arrays
            // Note: Non-object values are handled by the array's internal type checking
        }

        // Set element using unchecked access (bounds already verified)
        // PERFORMANCE: Eliminates redundant bounds check in array->set()
        try {
            array->setUnchecked(static_cast<size_t>(index), valueToSet);
        } catch (const std::runtime_error& e) {
            // Re-throw with source location information
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                std::string(e.what())
            );
        }
    }

    void ArrayExecutor::setFlatMultiArrayElement(std::shared_ptr<value::FlatMultiArray> flatArray, int64_t index, const value::Value& valueToSet) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), flatArray->size(), "FlatMultiArray");

        // For 1D arrays, set directly; multi-dimensional arrays cannot be set this way
        if (flatArray->getRank() == 1) {
            flatArray->set(static_cast<size_t>(index), valueToSet);
        } else {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Cannot set element in multi-dimensional FlatMultiArray with single index"
            );
        }
    }

    void ArrayExecutor::setSparseMultiArrayElement(std::shared_ptr<value::SparseMultiArray> sparseArray, int64_t index, const value::Value& valueToSet) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), sparseArray->size(), "SparseMultiArray");

        // For 1D arrays, set directly; multi-dimensional arrays cannot be set this way
        if (sparseArray->getRank() == 1) {
            std::vector<size_t> indices = {static_cast<size_t>(index)};
            sparseArray->set(indices, valueToSet);
        } else {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Cannot set element in multi-dimensional SparseMultiArray with single index"
            );
        }
    }

    void ArrayExecutor::handleArraySet() {
        // Pop value from stack
        value::Value valueToSet = context.stackManager->pop();

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int64_t index = std::get<int64_t>(indexVal);

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

        utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
            context,
            "ARRAY_SET: Invalid array type"
        );
    }

    void ArrayExecutor::handleArrayLength() {
        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();

        // Handle NativeArray
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayVal)) {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
            int64_t length = static_cast<int64_t>(array->size());
            context.stackManager->push(length);
            return;
        }

        // Handle FlatMultiArray
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(arrayVal)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(arrayVal);
            int64_t length = static_cast<int64_t>(flatArray->size());
            context.stackManager->push(length);
            return;
        }

        // Handle SparseMultiArray
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(arrayVal)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(arrayVal);
            int64_t length = static_cast<int64_t>(sparseArray->size());
            context.stackManager->push(length);
            return;
        }

        utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
            context,
            "ARRAY_LENGTH: Invalid array type"
        );
    }

    std::shared_ptr<value::NativeArray> ArrayExecutor::createJaggedArray(
        const std::vector<int64_t>& dimensions,
        size_t dimIndex,
        const std::string& elementTypeName,
        size_t totalDimensions)
    {
        if (dimIndex >= dimensions.size()) {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Invalid dimension index in jagged array creation"
            );
        }

        int64_t currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1) {
            // Last specified dimension
            // If this is also the last total dimension, create array of actual elements
            // Otherwise, create array of null references (for jagged arrays)
            auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;

            if (dimensions.size() == totalDimensions) {
                // Fully specified - create array of elements using ArrayFactory
                value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
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
                // Elements are initialized to null by default
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
        const std::vector<int64_t>& dimensions,
        size_t dimIndex,
        const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size()) {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Invalid dimension index in multi-dimensional array creation"
            );
        }

        int64_t currentDimSize = dimensions[dimIndex];
        auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;

        if (dimIndex == dimensions.size() - 1) {
            // Last dimension - create array of actual elements using ArrayFactory
            value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
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
        int64_t index = std::get<int64_t>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

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
        } else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(element)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(element);
            value::Value fieldValue = valueObj->getFieldValue(fieldName);
            context.stackManager->push(fieldValue);
        } else {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Cannot access field '" + fieldName + "' on non-object array element"
            );
        }
    }

    void ArrayExecutor::handleArraySetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get field name from constant pool
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop value to set from stack
        value::Value valueToSet = context.stackManager->pop();

        // Pop index from stack
        value::Value indexVal = context.stackManager->pop();
        int64_t index = std::get<int64_t>(indexVal);

        // Pop array from stack
        value::Value arrayVal = context.stackManager->pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check (VM does bounds check once)
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

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
        } else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(element)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(element);
            valueObj->setField(fieldName, valueToSet);
            // Write back modified value object to the array (value semantics)
            array->setUnchecked(arrayIndex, value::Value(valueObj));
        } else {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Cannot set field '" + fieldName + "' on non-object array element"
            );
        }
    }

    value::Value ArrayExecutor::createPooledMultiDimensionalArray(
        const std::vector<size_t>& dimensions,
        const std::string& elementTypeName)
    {
        // Get default value for the element type
        value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
        value::Value defaultValue;

        switch (elemType) {
            case value::ValueType::INT:
                defaultValue = 0;
                break;
            case value::ValueType::FLOAT:
                defaultValue = 0.0;
                break;
            case value::ValueType::BOOL:
                defaultValue = false;
                break;
            case value::ValueType::STRING:
                defaultValue = std::string("");
                break;
            default:
                defaultValue = nullptr;  // null for objects/arrays/lambdas
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

    void ArrayExecutor::handleArrayGetIntLocal(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t localSlot = instr.operands[0];
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

        value::Value indexVal = context.stackManager->pop();
        int64_t index = std::get<int64_t>(indexVal);

        const auto& array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
        context.stackManager->push(array->getIntDirect(static_cast<size_t>(index)));
    }

    void ArrayExecutor::handleArraySetIntLocal(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t localSlot = instr.operands[0];
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

        value::Value valToSet = context.stackManager->pop();
        value::Value indexVal = context.stackManager->pop();
        int64_t index = std::get<int64_t>(indexVal);
        int64_t val = std::get<int64_t>(valToSet);

        const auto& array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
        array->setIntDirect(static_cast<size_t>(index), val);
    }

    void ArrayExecutor::handleArrayLengthLocal(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t localSlot = instr.operands[0];
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        const value::Value& arrayVal = (*context.stackManager)[frameBase + localSlot];

        // Match handleArrayLength's dispatch: a local declared `int[][]` etc.
        // can hold a FlatMultiArray or SparseMultiArray (from NEW_ARRAY_MULTI's
        // pooled primitive path), not just a NativeArray. The previous
        // NativeArray-only std::get crashed with std::bad_variant_access when a
        // local multi-dim array's .length was read.
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayVal)) {
            const auto& array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);
            context.stackManager->push(static_cast<int64_t>(array->size()));
            return;
        }
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(arrayVal)) {
            const auto& array = std::get<std::shared_ptr<value::FlatMultiArray>>(arrayVal);
            context.stackManager->push(static_cast<int64_t>(array->size()));
            return;
        }
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(arrayVal)) {
            const auto& array = std::get<std::shared_ptr<value::SparseMultiArray>>(arrayVal);
            context.stackManager->push(static_cast<int64_t>(array->size()));
            return;
        }
        utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
            context,
            "ARRAY_LENGTH_LOCAL: Invalid array type"
        );
    }
}
