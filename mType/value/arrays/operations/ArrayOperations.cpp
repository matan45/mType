#include "ArrayOperations.hpp"
#include <algorithm>
#include <numeric>

namespace mType {
namespace value {
namespace arrays {

// Validation
bool ArrayOperations::areCompatible(const NativeArray& lhs, const NativeArray& rhs) {
    // Must have same size
    if (lhs.size() != rhs.size()) {
        return false;
    }

    // Must have compatible types
    ValueType lhsType = lhs.getElementType();
    ValueType rhsType = rhs.getElementType();

    // Same type is always compatible
    if (lhsType == rhsType) {
        return true;
    }

    // Numeric types are compatible with each other
    bool lhsNumeric = (lhsType == ValueType::INT || lhsType == ValueType::FLOAT);
    bool rhsNumeric = (rhsType == ValueType::INT || rhsType == ValueType::FLOAT);

    return lhsNumeric && rhsNumeric;
}

bool ArrayOperations::canApplyScalar(const NativeArray& array, const Value& scalar) {
    ValueType arrayType = array.getElementType();

    // Check if scalar type is compatible with array element type
    if (std::holds_alternative<int>(scalar)) {
        return arrayType == ValueType::INT || arrayType == ValueType::FLOAT;
    }
    if (std::holds_alternative<float>(scalar)) {
        return arrayType == ValueType::INT || arrayType == ValueType::FLOAT;
    }
    if (std::holds_alternative<bool>(scalar)) {
        return arrayType == ValueType::BOOL;
    }

    return false;
}

// Binary operations
std::shared_ptr<NativeArray> ArrayOperations::add(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    if (!areCompatible(lhs, rhs)) {
        throw std::runtime_error("Incompatible arrays for addition");
    }

    // Check if both use SIMD storage
    if (lhs.isSIMDOptimized() && rhs.isSIMDOptimized()) {
        if (lhs.getElementType() == ValueType::INT) {
            return addSIMDInt(lhs, rhs);
        } else if (lhs.getElementType() == ValueType::FLOAT) {
            return addSIMDFloat(lhs, rhs);
        }
    }

    // Fallback to heterogeneous
    return addHeterogeneous(lhs, rhs);
}

std::shared_ptr<NativeArray> ArrayOperations::addSIMDInt(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    auto lhsData = lhs.getSIMDIntData();
    auto rhsData = rhs.getSIMDIntData();

    if (!lhsData || !rhsData) {
        throw std::runtime_error("Expected SIMD int arrays");
    }

    size_t size = lhs.size();
    auto result = std::make_shared<NativeArray>(size, ValueType::INT);
    auto resultData = result->getSIMDIntData();

    // Use SIMD dispatcher for acceleration
    auto& dispatcher = simd::SIMDDispatcher::instance();
    dispatcher.addInt32(
        lhsData->data(),
        rhsData->data(),
        resultData->data(),
        size
    );

    return result;
}

std::shared_ptr<NativeArray> ArrayOperations::addSIMDFloat(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    auto lhsData = lhs.getSIMDFloatData();
    auto rhsData = rhs.getSIMDFloatData();

    if (!lhsData || !rhsData) {
        throw std::runtime_error("Expected SIMD float arrays");
    }

    size_t size = lhs.size();
    auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
    auto resultData = result->getSIMDFloatData();

    // Use SIMD dispatcher for acceleration
    auto& dispatcher = simd::SIMDDispatcher::instance();
    dispatcher.addFloat(
        lhsData->data(),
        rhsData->data(),
        resultData->data(),
        size
    );

    return result;
}

std::shared_ptr<NativeArray> ArrayOperations::addHeterogeneous(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    size_t size = lhs.size();
    auto result = std::make_shared<NativeArray>(size);

    for (size_t i = 0; i < size; ++i) {
        Value lhsVal = lhs.get(i);
        Value rhsVal = rhs.get(i);

        // Handle int + int
        if (std::holds_alternative<int>(lhsVal) && std::holds_alternative<int>(rhsVal)) {
            result->set(i, std::get<int>(lhsVal) + std::get<int>(rhsVal));
        }
        // Handle float + float
        else if (std::holds_alternative<float>(lhsVal) && std::holds_alternative<float>(rhsVal)) {
            result->set(i, std::get<float>(lhsVal) + std::get<float>(rhsVal));
        }
        // Handle int + float or float + int
        else if (std::holds_alternative<int>(lhsVal) && std::holds_alternative<float>(rhsVal)) {
            result->set(i, static_cast<float>(std::get<int>(lhsVal)) + std::get<float>(rhsVal));
        }
        else if (std::holds_alternative<float>(lhsVal) && std::holds_alternative<int>(rhsVal)) {
            result->set(i, std::get<float>(lhsVal) + static_cast<float>(std::get<int>(rhsVal)));
        }
        else {
            throw std::runtime_error("Unsupported type combination for addition");
        }
    }

    return result;
}

// Stub implementations for other operations (to be implemented)
std::shared_ptr<NativeArray> ArrayOperations::subtract(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    // Similar to add, but using subtraction
    throw std::runtime_error("Not yet implemented");
}

std::shared_ptr<NativeArray> ArrayOperations::multiply(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    throw std::runtime_error("Not yet implemented");
}

std::shared_ptr<NativeArray> ArrayOperations::divide(
    const NativeArray& lhs,
    const NativeArray& rhs
) {
    throw std::runtime_error("Not yet implemented");
}

// Scalar operations
std::shared_ptr<NativeArray> ArrayOperations::addScalar(
    const NativeArray& array,
    const Value& scalar
) {
    if (!canApplyScalar(array, scalar)) {
        throw std::runtime_error("Incompatible scalar type for array operation");
    }

    size_t size = array.size();

    // SIMD path for int arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::INT &&
        std::holds_alternative<int>(scalar)) {
        auto arrayData = array.getSIMDIntData();
        auto result = std::make_shared<NativeArray>(size, ValueType::INT);
        auto resultData = result->getSIMDIntData();

        auto& dispatcher = simd::SIMDDispatcher::instance();
        dispatcher.addInt32Scalar(
            arrayData->data(),
            std::get<int>(scalar),
            resultData->data(),
            size
        );

        return result;
    }

    // SIMD path for float arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::FLOAT &&
        std::holds_alternative<float>(scalar)) {
        auto arrayData = array.getSIMDFloatData();
        auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
        auto resultData = result->getSIMDFloatData();

        auto& dispatcher = simd::SIMDDispatcher::instance();
        dispatcher.addFloatScalar(
            arrayData->data(),
            std::get<float>(scalar),
            resultData->data(),
            size
        );

        return result;
    }

    // Fallback: element-wise addition
    auto result = std::make_shared<NativeArray>(size);
    for (size_t i = 0; i < size; ++i) {
        Value elemVal = array.get(i);

        if (std::holds_alternative<int>(elemVal) && std::holds_alternative<int>(scalar)) {
            result->set(i, std::get<int>(elemVal) + std::get<int>(scalar));
        } else if (std::holds_alternative<float>(elemVal) && std::holds_alternative<float>(scalar)) {
            result->set(i, std::get<float>(elemVal) + std::get<float>(scalar));
        }
    }

    return result;
}

std::shared_ptr<NativeArray> ArrayOperations::subtractScalar(
    const NativeArray& array,
    const Value& scalar,
    bool scalarOnLeft
) {
    throw std::runtime_error("Not yet implemented");
}

std::shared_ptr<NativeArray> ArrayOperations::multiplyScalar(
    const NativeArray& array,
    const Value& scalar
) {
    if (!canApplyScalar(array, scalar)) {
        throw std::runtime_error("Incompatible scalar type for array operation");
    }

    size_t size = array.size();

    // SIMD path for int arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::INT &&
        std::holds_alternative<int>(scalar)) {
        auto arrayData = array.getSIMDIntData();
        auto result = std::make_shared<NativeArray>(size, ValueType::INT);
        auto resultData = result->getSIMDIntData();

        auto& dispatcher = simd::SIMDDispatcher::instance();
        dispatcher.multiplyInt32Scalar(
            arrayData->data(),
            std::get<int>(scalar),
            resultData->data(),
            size
        );

        return result;
    }

    // SIMD path for float arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::FLOAT &&
        std::holds_alternative<float>(scalar)) {
        auto arrayData = array.getSIMDFloatData();
        auto result = std::make_shared<NativeArray>(size, ValueType::FLOAT);
        auto resultData = result->getSIMDFloatData();

        auto& dispatcher = simd::SIMDDispatcher::instance();
        dispatcher.multiplyFloatScalar(
            arrayData->data(),
            std::get<float>(scalar),
            resultData->data(),
            size
        );

        return result;
    }

    // Fallback
    auto result = std::make_shared<NativeArray>(size);
    for (size_t i = 0; i < size; ++i) {
        Value elemVal = array.get(i);

        if (std::holds_alternative<int>(elemVal) && std::holds_alternative<int>(scalar)) {
            result->set(i, std::get<int>(elemVal) * std::get<int>(scalar));
        } else if (std::holds_alternative<float>(elemVal) && std::holds_alternative<float>(scalar)) {
            result->set(i, std::get<float>(elemVal) * std::get<float>(scalar));
        }
    }

    return result;
}

std::shared_ptr<NativeArray> ArrayOperations::divideScalar(
    const NativeArray& array,
    const Value& scalar,
    bool scalarOnLeft
) {
    throw std::runtime_error("Not yet implemented");
}

// Reductions
Value ArrayOperations::sum(const NativeArray& array) {
    // SIMD path for int arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::INT) {
        auto arrayData = array.getSIMDIntData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.sumInt32(arrayData->data(), array.size()));
    }

    // SIMD path for float arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::FLOAT) {
        auto arrayData = array.getSIMDFloatData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.sumFloat(arrayData->data(), array.size()));
    }

    // Fallback
    int intSum = 0;
    float floatSum = 0.0f;
    bool isFloat = false;

    for (size_t i = 0; i < array.size(); ++i) {
        Value val = array.get(i);
        if (std::holds_alternative<int>(val)) {
            intSum += std::get<int>(val);
        } else if (std::holds_alternative<float>(val)) {
            floatSum += std::get<float>(val);
            isFloat = true;
        }
    }

    return isFloat ? Value(floatSum) : Value(intSum);
}

Value ArrayOperations::product(const NativeArray& array) {
    throw std::runtime_error("Not yet implemented");
}

Value ArrayOperations::min(const NativeArray& array) {
    // SIMD path for int arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::INT) {
        auto arrayData = array.getSIMDIntData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.minInt32(arrayData->data(), array.size()));
    }

    // SIMD path for float arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::FLOAT) {
        auto arrayData = array.getSIMDFloatData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.minFloat(arrayData->data(), array.size()));
    }

    throw std::runtime_error("Not yet implemented");
}

Value ArrayOperations::max(const NativeArray& array) {
    // SIMD path for int arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::INT) {
        auto arrayData = array.getSIMDIntData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.maxInt32(arrayData->data(), array.size()));
    }

    // SIMD path for float arrays
    if (array.isSIMDOptimized() && array.getElementType() == ValueType::FLOAT) {
        auto arrayData = array.getSIMDFloatData();
        auto& dispatcher = simd::SIMDDispatcher::instance();
        return Value(dispatcher.maxFloat(arrayData->data(), array.size()));
    }

    throw std::runtime_error("Not yet implemented");
}

Value ArrayOperations::average(const NativeArray& array) {
    Value sumVal = sum(array);

    if (std::holds_alternative<int>(sumVal)) {
        return Value(static_cast<float>(std::get<int>(sumVal)) / array.size());
    } else if (std::holds_alternative<float>(sumVal)) {
        return Value(std::get<float>(sumVal) / array.size());
    }

    throw std::runtime_error("Cannot compute average");
}

} // namespace arrays
} // namespace value
} // namespace mType
