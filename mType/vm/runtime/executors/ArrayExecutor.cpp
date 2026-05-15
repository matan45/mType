#include "ArrayExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>
#include "../../../environment/Environment.hpp"
#include "../../../environment/registry/ClassRegistry.hpp"
#include "../../../value/arrays/ArrayFactory.hpp"

namespace vm::runtime
{
    // MYT-319: hot dispatched handlers live in ArrayExecutor.hpp. Only array
    // creation (NEW_ARRAY / NEW_ARRAY_MULTI / buildMultiArray / jagged+nested
    // helpers) and the heavy setNativeArrayElement (with its full
    // ClassRegistry/InterfaceRegistry compat checking) stay here.

    void ArrayExecutor::handleNewArray(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        // Pop array size from stack
        value::Value sizeVal = context.stackManager->pop();
        int64_t size = value::asInt(sizeVal);

        if (size < 0) {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                "Array size cannot be negative: " + std::to_string(size)
            );
        }

        value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
        // Use ArrayFactory for optimized array creation
        auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;
        auto array = mType::value::arrays::ArrayFactory::create1DArray(
            static_cast<size_t>(size),
             elemType,
            elementTypeName,
            nullptr,  // ClassDefinition will be resolved by ArrayFactory
            classRegistry
        );

        context.stackManager->push(array);
    }

    void ArrayExecutor::handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        // Compiler emits: [typeIndex, dimensionCount (total), specifiedDimensions]
        size_t totalDimensions = instr.inlineOperands[1];
        size_t specifiedDimensions = instr.numOperands() > 2 ? instr.inlineOperands[2] : totalDimensions;

        // Pop dimension sizes from stack (in reverse order - last dimension first)
        std::vector<int64_t> dimensions;
        dimensions.reserve(specifiedDimensions);
        for (size_t i = 0; i < specifiedDimensions; ++i) {
            value::Value sizeVal = context.stackManager->pop();
            int64_t size = value::asInt(sizeVal);
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

    // MYT-146: pure-factory entry point - used by both handleNewArrayMulti and
    // VirtualMachine::createMultiArrayFromJit.
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

        // Pooled primitive multi-dim fast path.
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

    void ArrayExecutor::setNativeArrayElement(const std::shared_ptr<value::NativeArray>& array, int64_t index, const value::Value& valueToSet) {
        // Bounds check
        utils::ArrayBoundsChecker::checkBounds(context, static_cast<int>(index), array->size(), "Array");

        // Type check for object arrays with generics
        if (array->getElementType() == value::ValueType::OBJECT) {
            std::string expectedTypeName = array->getElementTypeName();

            // MYT-281: `Object[]` is the universal heap-typed slot.
            const bool isObjectSlot = (expectedTypeName == "Object");

            // Check for array-to-array assignment.
            if (!isObjectSlot && value::isNativeArray(valueToSet)) {
                auto valueArray = value::asNativeArray(valueToSet);
                std::string actualArrayElementType = valueArray->getElementTypeName();
                value::ValueType actualElementValueType = valueArray->getElementType();

                if (index < static_cast<int>(array->size())) {
                    value::Value existingElement = array->get(static_cast<size_t>(index));

                    if (value::isNativeArray(existingElement)) {
                        auto existingArray = value::asNativeArray(existingElement);
                        std::string existingElementType = existingArray->getElementTypeName();
                        value::ValueType existingElementValueType = existingArray->getElementType();

                        bool isPrimitiveType = (actualElementValueType == value::ValueType::INT ||
                                              actualElementValueType == value::ValueType::FLOAT ||
                                              actualElementValueType == value::ValueType::STRING ||
                                              actualElementValueType == value::ValueType::BOOL);

                        if (isPrimitiveType) {
                            if (existingElementType != actualArrayElementType) {
                                utils::ErrorLocationHelper::throwError<errors::TypeException>(
                                    context,
                                    "Array element type mismatch: cannot assign " + actualArrayElementType +
                                    "[] to slot expecting " + existingElementType + "[]"
                                );
                            }
                        } else if (actualElementValueType == value::ValueType::OBJECT &&
                                   existingElementValueType == value::ValueType::OBJECT) {
                            if (existingElementType != actualArrayElementType) {
                                bool isCompatible = false;

                                if (context.environment) {
                                    auto classRegistry = context.environment->getClassRegistry();
                                    auto actualClass = classRegistry->findClass(actualArrayElementType);

                                    if (actualClass) {
                                        auto currentClass = actualClass;
                                        while (currentClass && currentClass->hasParentClass()) {
                                            if (currentClass->getParentClassName() == existingElementType) {
                                                isCompatible = true;
                                                break;
                                            }
                                            currentClass = classRegistry->findClass(currentClass->getParentClassName());
                                        }

                                        if (!isCompatible) {
                                            auto interfaceRegistry = context.environment->getInterfaceRegistry();
                                            auto interfaceDef = interfaceRegistry->findInterface(existingElementType);

                                            if (interfaceDef) {
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

            // Only enforce strict type checking for generic types with type arguments.
            bool isGenericInstantiation = (expectedTypeName.find('<') != std::string::npos);

            if (value::isObject(valueToSet)) {
                auto objInstance = value::asObject(valueToSet);

                std::string actualTypeName = objInstance->getFullTypeName();

                if (isGenericInstantiation) {
                    if (expectedTypeName != actualTypeName) {
                        utils::ErrorLocationHelper::throwError<errors::TypeException>(
                            context,
                            "Array element type mismatch: cannot assign " + actualTypeName +
                            " to array of type " + expectedTypeName + "[]"
                        );
                    }
                }
            }
        }

        // Set element using unchecked access (bounds already verified).
        try {
            array->setUnchecked(static_cast<size_t>(index), valueToSet);
        } catch (const std::runtime_error& e) {
            utils::ErrorLocationHelper::throwError<errors::RuntimeException>(
                context,
                std::string(e.what())
            );
        }
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
            auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;

            if (dimensions.size() == totalDimensions) {
                value::ValueType elemType = ::types::TypeConversionUtils::stringToValueType(elementTypeName);
                return mType::value::arrays::ArrayFactory::create1DArray(
                    currentDimSize,
                    elemType,
                    elementTypeName,
                    nullptr,
                    classRegistry
                );
            } else {
                auto array = mType::value::arrays::ArrayFactory::create1DArray(
                    currentDimSize,
                    value::ValueType::OBJECT,
                    "Array",
                    nullptr,
                    classRegistry
                );
                return array;
            }
        }
        else {
            auto classRegistry = context.environment ? context.environment->getClassRegistry().get() : nullptr;
            auto outerArray = mType::value::arrays::ArrayFactory::create1DArray(
                currentDimSize,
                value::ValueType::OBJECT,
                "Array",
                nullptr,
                classRegistry
            );

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
            auto outerArray = mType::value::arrays::ArrayFactory::create1DArray(
                currentDimSize,
                value::ValueType::OBJECT,
                "Array",
                nullptr,
                classRegistry
            );

            for (int i = 0; i < currentDimSize; ++i) {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    value::Value ArrayExecutor::createPooledMultiDimensionalArray(
        const std::vector<size_t>& dimensions,
        const std::string& elementTypeName)
    {
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
                defaultValue = nullptr;
                break;
        }

        auto& pool = value::ArrayPool::getInstance();
        return pool.acquireAdaptive(dimensions, defaultValue);
    }
}
