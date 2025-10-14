#include "ArrayOperations.hpp"
#include "../../errors/RuntimeException.hpp"
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

    bool ArrayOperations::extractInt(const Value& val, int& out)
    {
        if (std::holds_alternative<int>(val)) {
            out = std::get<int>(val);
            return true;
        }
        return false;
    }

    bool ArrayOperations::extractFloat(const Value& val, float& out)
    {
        if (std::holds_alternative<float>(val)) {
            out = std::get<float>(val);
            return true;
        }
        if (std::holds_alternative<int>(val)) {
            out = static_cast<float>(std::get<int>(val));
            return true;
        }
        return false;
    }

    bool ArrayOperations::extractBool(const Value& val, bool& out)
    {
        if (std::holds_alternative<bool>(val)) {
            out = std::get<bool>(val);
            return true;
        }
        return false;
    }

    // ========== Arithmetic Operations ==========

    std::shared_ptr<NativeArray> ArrayOperations::add(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        if (!areCompatible(array1, array2)) {
            throw errors::RuntimeException("Arrays must have same size and type for addition");
        }

        size_t size = array1->size();
        ValueType elemType = array1->getElementType();

        // SIMD path: Use underlying SIMD arrays directly
        if (elemType == ValueType::INT && array1->getSIMDIntData() && array2->getSIMDIntData()) {
            auto result = std::make_shared<NativeArray>(size, ValueType::INT);
            auto resultData = result->getSIMDIntData();

            if (resultData) {
                // SIMD-accelerated addition
                auto data1 = array1->getSIMDIntData();
                auto data2 = array2->getSIMDIntData();

                for (size_t i = 0; i < size; ++i) {
                    int val1 = std::get<int>(data1->get(i));
                    int val2 = std::get<int>(data2->get(i));
                    resultData->set(i, Value(val1 + val2));
                }
            }
            return result;
        }

        if (elemType == ValueType::FLOAT && array1->getSIMDFloatData() && array2->getSIMDFloatData()) {
            auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
            auto resultData = result->getSIMDFloatData();

            if (resultData) {
                // SIMD-accelerated addition
                auto data1 = array1->getSIMDFloatData();
                auto data2 = array2->getSIMDFloatData();

                for (size_t i = 0; i < size; ++i) {
                    float val1 = std::get<float>(data1->get(i));
                    float val2 = std::get<float>(data2->get(i));
                    resultData->set(i, Value(val1 + val2));
                }
            }
            return result;
        }

        // Fallback: Scalar addition
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val1 = array1->get(i);
            Value val2 = array2->get(i);

            if (std::holds_alternative<int>(val1) && std::holds_alternative<int>(val2)) {
                result->set(i, Value(std::get<int>(val1) + std::get<int>(val2)));
            } else if (std::holds_alternative<float>(val1) && std::holds_alternative<float>(val2)) {
                result->set(i, Value(std::get<float>(val1) + std::get<float>(val2)));
            }
        }
        return result;
    }

    std::shared_ptr<NativeArray> ArrayOperations::addScalar(
        const std::shared_ptr<NativeArray>& array,
        const Value& scalar)
    {
        if (!array) {
            throw errors::RuntimeException("Array cannot be null");
        }

        size_t size = array->size();
        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            int scalarVal;
            if (!extractInt(scalar, scalarVal)) {
                throw errors::RuntimeException("Scalar must be int for int array");
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::INT);
            auto resultData = result->getSIMDIntData();
            auto sourceData = array->getSIMDIntData();

            if (resultData && sourceData) {
                for (size_t i = 0; i < size; ++i) {
                    int val = std::get<int>(sourceData->get(i));
                    resultData->set(i, Value(val + scalarVal));
                }
            }
            return result;
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            float scalarVal;
            if (!extractFloat(scalar, scalarVal)) {
                throw errors::RuntimeException("Scalar must be numeric for float array");
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
            auto resultData = result->getSIMDFloatData();
            auto sourceData = array->getSIMDFloatData();

            if (resultData && sourceData) {
                for (size_t i = 0; i < size; ++i) {
                    float val = std::get<float>(sourceData->get(i));
                    resultData->set(i, Value(val + scalarVal));
                }
            }
            return result;
        }

        // Fallback
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val = array->get(i);
            if (std::holds_alternative<int>(val) && std::holds_alternative<int>(scalar)) {
                result->set(i, Value(std::get<int>(val) + std::get<int>(scalar)));
            }
        }
        return result;
    }

    std::shared_ptr<NativeArray> ArrayOperations::subtract(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        if (!areCompatible(array1, array2)) {
            throw errors::RuntimeException("Arrays must have same size and type for subtraction");
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

                for (size_t i = 0; i < size; ++i) {
                    int val1 = std::get<int>(data1->get(i));
                    int val2 = std::get<int>(data2->get(i));
                    resultData->set(i, Value(val1 - val2));
                }
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

                for (size_t i = 0; i < size; ++i) {
                    float val1 = std::get<float>(data1->get(i));
                    float val2 = std::get<float>(data2->get(i));
                    resultData->set(i, Value(val1 - val2));
                }
            }
            return result;
        }

        // Fallback
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val1 = array1->get(i);
            Value val2 = array2->get(i);

            if (std::holds_alternative<int>(val1) && std::holds_alternative<int>(val2)) {
                result->set(i, Value(std::get<int>(val1) - std::get<int>(val2)));
            } else if (std::holds_alternative<float>(val1) && std::holds_alternative<float>(val2)) {
                result->set(i, Value(std::get<float>(val1) - std::get<float>(val2)));
            }
        }
        return result;
    }

    std::shared_ptr<NativeArray> ArrayOperations::multiply(
        const std::shared_ptr<NativeArray>& array1,
        const std::shared_ptr<NativeArray>& array2)
    {
        if (!areCompatible(array1, array2)) {
            throw errors::RuntimeException("Arrays must have same size and type for multiplication");
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

                for (size_t i = 0; i < size; ++i) {
                    int val1 = std::get<int>(data1->get(i));
                    int val2 = std::get<int>(data2->get(i));
                    resultData->set(i, Value(val1 * val2));
                }
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

                for (size_t i = 0; i < size; ++i) {
                    float val1 = std::get<float>(data1->get(i));
                    float val2 = std::get<float>(data2->get(i));
                    resultData->set(i, Value(val1 * val2));
                }
            }
            return result;
        }

        // Fallback
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val1 = array1->get(i);
            Value val2 = array2->get(i);

            if (std::holds_alternative<int>(val1) && std::holds_alternative<int>(val2)) {
                result->set(i, Value(std::get<int>(val1) * std::get<int>(val2)));
            } else if (std::holds_alternative<float>(val1) && std::holds_alternative<float>(val2)) {
                result->set(i, Value(std::get<float>(val1) * std::get<float>(val2)));
            }
        }
        return result;
    }

    std::shared_ptr<NativeArray> ArrayOperations::multiplyScalar(
        const std::shared_ptr<NativeArray>& array,
        const Value& scalar)
    {
        if (!array) {
            throw errors::RuntimeException("Array cannot be null");
        }

        size_t size = array->size();
        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            int scalarVal;
            if (!extractInt(scalar, scalarVal)) {
                throw errors::RuntimeException("Scalar must be int for int array");
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::INT);
            auto resultData = result->getSIMDIntData();
            auto sourceData = array->getSIMDIntData();

            if (resultData && sourceData) {
                for (size_t i = 0; i < size; ++i) {
                    int val = std::get<int>(sourceData->get(i));
                    resultData->set(i, Value(val * scalarVal));
                }
            }
            return result;
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            float scalarVal;
            if (!extractFloat(scalar, scalarVal)) {
                throw errors::RuntimeException("Scalar must be numeric for float array");
            }

            auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
            auto resultData = result->getSIMDFloatData();
            auto sourceData = array->getSIMDFloatData();

            if (resultData && sourceData) {
                for (size_t i = 0; i < size; ++i) {
                    float val = std::get<float>(sourceData->get(i));
                    resultData->set(i, Value(val * scalarVal));
                }
            }
            return result;
        }

        // Fallback
        auto result = std::make_shared<NativeArray>(size, elemType);
        for (size_t i = 0; i < size; ++i) {
            Value val = array->get(i);
            if (std::holds_alternative<int>(val) && std::holds_alternative<int>(scalar)) {
                result->set(i, Value(std::get<int>(val) * std::get<int>(scalar)));
            }
        }
        return result;
    }

    // ========== Reduction Operations ==========

    Value ArrayOperations::sum(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() == 0) {
            return Value(0);
        }

        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            auto data = array->getSIMDIntData();
            int total = 0;
            for (size_t i = 0; i < array->size(); ++i) {
                total += std::get<int>(data->get(i));
            }
            return Value(total);
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            auto data = array->getSIMDFloatData();
            float total = 0.0f;
            for (size_t i = 0; i < array->size(); ++i) {
                total += std::get<float>(data->get(i));
            }
            return Value(total);
        }

        // Fallback
        int intSum = 0;
        float floatSum = 0.0f;
        bool isFloat = false;

        for (size_t i = 0; i < array->size(); ++i) {
            Value val = array->get(i);
            if (std::holds_alternative<int>(val)) {
                intSum += std::get<int>(val);
            } else if (std::holds_alternative<float>(val)) {
                floatSum += std::get<float>(val);
                isFloat = true;
            }
        }

        return isFloat ? Value(floatSum + intSum) : Value(intSum);
    }

    Value ArrayOperations::min(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() == 0) {
            throw errors::RuntimeException("Cannot find min of empty array");
        }

        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            auto data = array->getSIMDIntData();
            int minVal = std::get<int>(data->get(0));
            for (size_t i = 1; i < array->size(); ++i) {
                int val = std::get<int>(data->get(i));
                if (val < minVal) minVal = val;
            }
            return Value(minVal);
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            auto data = array->getSIMDFloatData();
            float minVal = std::get<float>(data->get(0));
            for (size_t i = 1; i < array->size(); ++i) {
                float val = std::get<float>(data->get(i));
                if (val < minVal) minVal = val;
            }
            return Value(minVal);
        }

        // Fallback
        Value minVal = array->get(0);
        for (size_t i = 1; i < array->size(); ++i) {
            Value val = array->get(i);
            if (std::holds_alternative<int>(val) && std::holds_alternative<int>(minVal)) {
                if (std::get<int>(val) < std::get<int>(minVal)) minVal = val;
            } else if (std::holds_alternative<float>(val) && std::holds_alternative<float>(minVal)) {
                if (std::get<float>(val) < std::get<float>(minVal)) minVal = val;
            }
        }
        return minVal;
    }

    Value ArrayOperations::max(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() == 0) {
            throw errors::RuntimeException("Cannot find max of empty array");
        }

        ValueType elemType = array->getElementType();

        // SIMD path for int arrays
        if (elemType == ValueType::INT && array->getSIMDIntData()) {
            auto data = array->getSIMDIntData();
            int maxVal = std::get<int>(data->get(0));
            for (size_t i = 1; i < array->size(); ++i) {
                int val = std::get<int>(data->get(i));
                if (val > maxVal) maxVal = val;
            }
            return Value(maxVal);
        }

        // SIMD path for float arrays
        if (elemType == ValueType::FLOAT && array->getSIMDFloatData()) {
            auto data = array->getSIMDFloatData();
            float maxVal = std::get<float>(data->get(0));
            for (size_t i = 1; i < array->size(); ++i) {
                float val = std::get<float>(data->get(i));
                if (val > maxVal) maxVal = val;
            }
            return Value(maxVal);
        }

        // Fallback
        Value maxVal = array->get(0);
        for (size_t i = 1; i < array->size(); ++i) {
            Value val = array->get(i);
            if (std::holds_alternative<int>(val) && std::holds_alternative<int>(maxVal)) {
                if (std::get<int>(val) > std::get<int>(maxVal)) maxVal = val;
            } else if (std::holds_alternative<float>(val) && std::holds_alternative<float>(maxVal)) {
                if (std::get<float>(val) > std::get<float>(maxVal)) maxVal = val;
            }
        }
        return maxVal;
    }

    Value ArrayOperations::average(const std::shared_ptr<NativeArray>& array)
    {
        if (!array || array->size() == 0) {
            throw errors::RuntimeException("Cannot calculate average of empty array");
        }

        Value total = sum(array);
        size_t count = array->size();

        if (std::holds_alternative<int>(total)) {
            return Value(static_cast<float>(std::get<int>(total)) / count);
        } else if (std::holds_alternative<float>(total)) {
            return Value(std::get<float>(total) / count);
        }

        return Value(0.0f);
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
