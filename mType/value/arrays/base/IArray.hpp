#pragma once
#include <cstddef>
#include <string>
#include <memory>
#include "../../ValueType.hpp"

namespace mType {
namespace value {
namespace arrays {

/**
 * Abstract interface for all array types in mType.
 * Provides common operations while allowing specialized implementations.
 *
 * Design Principles:
 * - Interface Segregation: Minimal interface, specialized via inheritance
 * - Liskov Substitution: All implementations substitutable
 * - Dependency Inversion: High-level code depends on this abstraction
 */
class IArray {
public:
    virtual ~IArray() = default;

    // Core properties
    virtual size_t size() const = 0;
    virtual size_t capacity() const = 0;
    virtual bool empty() const { return size() == 0; }
    virtual std::string elementTypeName() const = 0;

    // Element access (returns Value type)
    virtual ::value::Value get(size_t index) const = 0;
    virtual void set(size_t index, const ::value::Value& value) = 0;

    // Memory management
    virtual void reserve(size_t newCapacity) = 0;
    virtual void resize(size_t newSize) = 0;
    virtual void clear() = 0;

    // SIMD capability queries
    virtual bool supportsSIMD() const = 0;
    virtual size_t simdWidth() const { return 1; }

    // Factory method for copies
    virtual std::unique_ptr<IArray> clone() const = 0;

    // Type identification
    virtual ::value::ValueType elementType() const = 0;
};

} // namespace arrays
} // namespace value
} // namespace mType
