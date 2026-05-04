#include "ArrayOperations.hpp"
#include <cstdint>
#include "../../errors/RuntimeException.hpp"
#include "../simd/SIMDOperations.hpp"
#include "../ValueShim.hpp"
#include <algorithm>
#include <cmath>

namespace value::operations
{
    // ========== Helper Functions ==========

    bool ArrayOperations::areCompatible(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        if (!array1 || !array2) return false;
        if (array1->size() != array2->size()) return false;
        if (array1->getElementType() != array2->getElementType()) return false;
        return true;
    }

    bool ArrayOperations::extractInt(const Value& val, int64_t& out)
    {
        if (value::isInt(val)) {
            out = value::asInt(val);
            return true;
        }
        return false;
    }

    bool ArrayOperations::extractFloat(const Value& val, double& out)
    {
        if (value::isFloat(val)) {
            out = value::asFloat(val);
            return true;
        }
        if (value::isInt(val)) {
            out = static_cast<double>(value::asInt(val));
            return true;
        }
        return false;
    }

    bool ArrayOperations::extractBool(const Value& val, bool& out)
    {
        if (value::isBool(val)) {
            out = value::asBool(val);
            return true;
        }
        return false;
    }

    // ========== Template Helper Implementations ==========

    template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
    std::shared_ptr<NativeArray> ArrayOperations::performBinaryOperation(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2,
        IntSimdOp intSimdOp,
        FloatSimdOp floatSimdOp,
        ScalarOp scalarOp,
        const char* operationName)
    {
        if (!areCompatible(array1, array2)) {
            throw errors::RuntimeException(
                std::string("Arrays must have same size and type for ") + operationName);
        }

        size_t size = array1->size();
        ValueType elemType = array1->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array1->getSIMDIntData() && array2->getSIMDIntData()) {
            auto result = std::make_shared<NativeArray>(size, ValueType::INT);
            auto resultData = result->getSIMDIntData();

            if (resultData) {
                auto data1 = array1->getSIMDIntData();
                auto data2 = array2->getSIMDIntData();
                intSimdOp(data1->data(), data2->data(), resultData->data(), size);
            }
            return result;
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array1->getSIMDFloatData() && array2->getSIMDFloatData()) {
            auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
            auto resultData = result->getSIMDFloatData();

            if (resultData) {
                auto data1 = array1->getSIMDFloatData();
                auto data2 = array2->getSIMDFloatData();
                floatSimdOp(data1->data(), data2->data(), resultData->data(), size);
            }
            return result;
        }

        // Fallback: Scalar operation
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val1 = array1->get(i);
            Value val2 = array2->get(i);

            if (value::isInt(val1) && value::isInt(val2)) {
                result->set(i, Value(scalarOp(value::asInt(val1), value::asInt(val2))));
            } else if (value::isFloat(val1) && value::isFloat(val2)) {
                result->set(i, Value(scalarOp(value::asFloat(val1), value::asFloat(val2))));
            }
        }
        return result;
    }

    template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
    std::shared_ptr<NativeArray> ArrayOperations::performScalarOperation(
        const std::shared_ptr<NativeArray>& array,
        const Value& scalar,
        IntSimdOp intSimdOp,
        FloatSimdOp floatSimdOp,
        ScalarOp scalarOp,
        const char* operationName)
    {
        if (!array) {
            throw errors::RuntimeException("Array cannot be null");
        }

        size_t size = array->size();
        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            int64_t scalarVal;
            if (!extractInt(scalar, scalarVal)) {
                throw errors::RuntimeException(
                    std::string("Scalar must be int for int array in ") + operationName);
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::INT);
            auto resultData = result->getSIMDIntData();
            auto sourceData = array->getSIMDIntData();

            if (resultData && sourceData) {
                intSimdOp(sourceData->data(), scalarVal, resultData->data(), size);
            }
            return result;
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            double scalarVal;
            if (!extractFloat(scalar, scalarVal)) {
                throw errors::RuntimeException(
                    std::string("Scalar must be numeric for float array in ") + operationName);
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
            auto resultData = result->getSIMDFloatData();
            auto sourceData = array->getSIMDFloatData();

            if (resultData && sourceData) {
                floatSimdOp(sourceData->data(), scalarVal, resultData->data(), size);
            }
            return result;
        }

        // Fallback: Scalar operation
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val = array->get(i);
            if (value::isInt(val) && value::isInt(scalar)) {
                result->set(i, Value(scalarOp(value::asInt(val), value::asInt(scalar))));
            } else if (value::isFloat(val) && value::isFloat(scalar)) {
                result->set(i, Value(scalarOp(value::asFloat(val), value::asFloat(scalar))));
            }
        }
        return result;
    }

    template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
    Value ArrayOperations::performReduction(
        const std::shared_ptr<NativeArray>& array,
        IntSimdOp intSimdOp,
        FloatSimdOp floatSimdOp,
        ScalarOp scalarOp,
        const char* operationName,
        bool allowEmpty)
    {
        if (!array || array->size() == 0) {
            if (allowEmpty) {
                return Value(0);
            }
            throw errors::RuntimeException(
                std::string("Cannot perform ") + operationName + " on empty array");
        }

        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            auto data = array->getSIMDIntData();
            int64_t result = intSimdOp(data->data(), array->size());
            return Value(result);
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            auto data = array->getSIMDFloatData();
            double result = floatSimdOp(data->data(), array->size());
            return Value(result);
        }

        // Fallback: Use scalar operation
        return scalarOp(array);
    }

    // ========== Arithmetic Operations ==========

    std::shared_ptr<NativeArray> ArrayOperations::add(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        return performBinaryOperation(
            array1, array2,
            mType::value::simd::SIMDOperations::addInt,
            mType::value::simd::SIMDOperations::addFloat,
            [](auto a, auto b) { return a + b; },
            "addition"
        );
    }

    std::shared_ptr<NativeArray> ArrayOperations::addScalar(
        const std::shared_ptr<NativeArray>& array,
        const Value& scalar)
    {
        return performScalarOperation(
            array, scalar,
            mType::value::simd::SIMDOperations::addScalarInt,
            mType::value::simd::SIMDOperations::addScalarFloat,
            [](auto a, auto b) { return a + b; },
            "scalar addition"
        );
    }

    std::shared_ptr<NativeArray> ArrayOperations::subtract(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        return performBinaryOperation(
            array1, array2,
            mType::value::simd::SIMDOperations::subtractInt,
            mType::value::simd::SIMDOperations::subtractFloat,
            [](auto a, auto b) { return a - b; },
            "subtraction"
        );
    }

    std::shared_ptr<NativeArray> ArrayOperations::multiply(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        return performBinaryOperation(
            array1, array2,
            mType::value::simd::SIMDOperations::multiplyInt,
            mType::value::simd::SIMDOperations::multiplyFloat,
            [](auto a, auto b) { return a * b; },
            "multiplication"
        );
    }

    std::shared_ptr<NativeArray> ArrayOperations::multiplyScalar(
        const std::shared_ptr<NativeArray>& array,
        const Value& scalar)
    {
        return performScalarOperation(
            array, scalar,
            mType::value::simd::SIMDOperations::multiplyScalarInt,
            mType::value::simd::SIMDOperations::multiplyScalarFloat,
            [](auto a, auto b) { return a * b; },
            "scalar multiplication"
        );
    }

    // ========== Reduction Operations ==========

    Value ArrayOperations::sum(const std::shared_ptr<NativeArray>& array)
    {
        return performReduction(
            array,
            mType::value::simd::SIMDOperations::sumInt,
            mType::value::simd::SIMDOperations::sumFloat,
            [](const std::shared_ptr<NativeArray>& arr) -> Value {
                int64_t intSum = 0;
                double floatSum = 0.0;
                bool isFloat = false;

                for (size_t i = 0; i < arr->size(); ++i) {
                    Value val = arr->get(i);
                    if (value::isInt(val)) {
                        intSum += value::asInt(val);
                    } else if (value::isFloat(val)) {
                        floatSum += value::asFloat(val);
                        isFloat = true;
                    }
                }
                return isFloat ? Value(floatSum + static_cast<double>(intSum)) : Value(intSum);
            },
            "sum",
            true  // allowEmpty
        );
    }

    Value ArrayOperations::min(const std::shared_ptr<NativeArray>& array)
    {
        return performReduction(
            array,
            mType::value::simd::SIMDOperations::minInt,
            mType::value::simd::SIMDOperations::minFloat,
            [](const std::shared_ptr<NativeArray>& arr) -> Value {
                Value minVal = arr->get(0);
                for (size_t i = 1; i < arr->size(); ++i) {
                    Value val = arr->get(i);
                    if (value::isInt(val) && value::isInt(minVal)) {
                        if (value::asInt(val) < value::asInt(minVal)) minVal = val;
                    } else if (value::isFloat(val) && value::isFloat(minVal)) {
                        if (value::asFloat(val) < value::asFloat(minVal)) minVal = val;
                    }
                }
                return minVal;
            },
            "min",
            false  // don't allowEmpty
        );
    }

    Value ArrayOperations::max(const std::shared_ptr<NativeArray>& array)
    {
        return performReduction(
            array,
            mType::value::simd::SIMDOperations::maxInt,
            mType::value::simd::SIMDOperations::maxFloat,
            [](const std::shared_ptr<NativeArray>& arr) -> Value {
                Value maxVal = arr->get(0);
                for (size_t i = 1; i < arr->size(); ++i) {
                    Value val = arr->get(i);
                    if (value::isInt(val) && value::isInt(maxVal)) {
                        if (value::asInt(val) > value::asInt(maxVal)) maxVal = val;
                    } else if (value::isFloat(val) && value::isFloat(maxVal)) {
                        if (value::asFloat(val) > value::asFloat(maxVal)) maxVal = val;
                    }
                }
                return maxVal;
            },
            "max",
            false  // don't allowEmpty
        );
    }

    Value ArrayOperations::average(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() == 0) {
            throw errors::RuntimeException("Cannot calculate average of empty array");
        }

        Value total = sum(array);
        size_t count = array->size();

        if (value::isInt(total)) {
            return Value(static_cast<double>(value::asInt(total)) / static_cast<double>(count));
        } else if (value::isFloat(total)) {
            return Value(value::asFloat(total) / count);
        }

        return Value(0.0);
    }

    // ========== Utility Operations ==========

    void ArrayOperations::fill(
        const std::shared_ptr<NativeArray>& array,
        const Value& value)
    {
        if (!array) return;

        for (size_t i = 0; i < array->size(); ++i) {
            array->set(i, value);
        }
    }

    std::shared_ptr<NativeArray> ArrayOperations::copy(
        const std::shared_ptr<NativeArray>& source)
    {
        if (!source) return nullptr;

        auto result = std::make_shared<NativeArray>(source->size(), source->getElementType());
        for (size_t i = 0; i < source->size(); ++i) {
            result->set(i, source->get(i));
        }
        return result;
    }

    void ArrayOperations::reverse(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() <= 1) return;

        size_t left = 0;
        size_t right = array->size() - 1;

        while (left < right) {
            Value temp = array->get(left);
            array->set(left, array->get(right));
            array->set(right, temp);
            ++left;
            --right;
        }
    }
    
} // namespace value::operations
