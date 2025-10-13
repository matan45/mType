#include "ScalarOps.hpp"
#include <algorithm>
#include <limits>

namespace mType {
namespace value {
namespace simd {
namespace scalar {

// Integer operations
void ScalarOps::addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

void ScalarOps::subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] - b[i];
    }
}

void ScalarOps::multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
}

void ScalarOps::divideInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] / b[i];  // Note: Division by zero not checked for performance
    }
}

void ScalarOps::addInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + scalar;
    }
}

void ScalarOps::multiplyInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] * scalar;
    }
}

// Integer reductions
int32_t ScalarOps::sumInt32(const int32_t* data, size_t count) {
    int32_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += data[i];
    }
    return sum;
}

int32_t ScalarOps::minInt32(const int32_t* data, size_t count) {
    if (count == 0) return std::numeric_limits<int32_t>::max();
    int32_t minVal = data[0];
    for (size_t i = 1; i < count; ++i) {
        if (data[i] < minVal) minVal = data[i];
    }
    return minVal;
}

int32_t ScalarOps::maxInt32(const int32_t* data, size_t count) {
    if (count == 0) return std::numeric_limits<int32_t>::min();
    int32_t maxVal = data[0];
    for (size_t i = 1; i < count; ++i) {
        if (data[i] > maxVal) maxVal = data[i];
    }
    return maxVal;
}

// Float operations
void ScalarOps::addFloat(const float* a, const float* b, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

void ScalarOps::subtractFloat(const float* a, const float* b, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] - b[i];
    }
}

void ScalarOps::multiplyFloat(const float* a, const float* b, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
}

void ScalarOps::divideFloat(const float* a, const float* b, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] / b[i];
    }
}

void ScalarOps::addFloatScalar(const float* a, float scalar, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + scalar;
    }
}

void ScalarOps::multiplyFloatScalar(const float* a, float scalar, float* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] * scalar;
    }
}

// Float reductions
float ScalarOps::sumFloat(const float* data, size_t count) {
    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += data[i];
    }
    return sum;
}

float ScalarOps::minFloat(const float* data, size_t count) {
    if (count == 0) return std::numeric_limits<float>::max();
    float minVal = data[0];
    for (size_t i = 1; i < count; ++i) {
        if (data[i] < minVal) minVal = data[i];
    }
    return minVal;
}

float ScalarOps::maxFloat(const float* data, size_t count) {
    if (count == 0) return std::numeric_limits<float>::lowest();
    float maxVal = data[0];
    for (size_t i = 1; i < count; ++i) {
        if (data[i] > maxVal) maxVal = data[i];
    }
    return maxVal;
}

// Boolean operations
void ScalarOps::andBool(const bool* a, const bool* b, bool* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] && b[i];
    }
}

void ScalarOps::orBool(const bool* a, const bool* b, bool* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] || b[i];
    }
}

void ScalarOps::notBool(const bool* a, bool* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = !a[i];
    }
}

bool ScalarOps::anyTrue(const bool* data, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (data[i]) return true;
    }
    return false;
}

bool ScalarOps::allTrue(const bool* data, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!data[i]) return false;
    }
    return true;
}

} // namespace scalar
} // namespace simd
} // namespace value
} // namespace mType
