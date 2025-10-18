#include "InternedString.hpp"
#include "StringPool.hpp"

namespace value
{
    // Private constructor for StringPool to create instances
    InternedString::InternedString(size_t id, StringPool* p)
        : poolId(id), pool(p) {}

    // Default constructor creates empty string (ID = 0)
    InternedString::InternedString()
        : poolId(0), pool(nullptr) {}

    // Copy constructor with reference counting
    InternedString::InternedString(const InternedString& other)
        : poolId(other.poolId), pool(other.pool)
    {
        if (pool && poolId != 0) {
            pool->incrementRef(poolId);
        }
    }

    // Move constructor (efficient, no ref count change)
    InternedString::InternedString(InternedString&& other) noexcept
        : poolId(other.poolId), pool(other.pool)
    {
        other.poolId = 0;
        other.pool = nullptr;
    }

    // Copy assignment with reference counting
    InternedString& InternedString::operator=(const InternedString& other)
    {
        if (this != &other) {
            if (pool && poolId != 0) {
                pool->decrementRef(poolId);
            }

            poolId = other.poolId;
            pool = other.pool;

            if (pool && poolId != 0) {
                pool->incrementRef(poolId);
            }
        }
        return *this;
    }

    // Move assignment (efficient, no ref count change)
    InternedString& InternedString::operator=(InternedString&& other) noexcept
    {
        if (this != &other) {
            if (pool && poolId != 0) {
                pool->decrementRef(poolId);
            }

            poolId = other.poolId;
            pool = other.pool;

            other.poolId = 0;
            other.pool = nullptr;
        }
        return *this;
    }

    // Destructor decrements reference count
    InternedString::~InternedString()
    {
        if (pool && poolId != 0) {
            pool->decrementRef(poolId);
        }
    }

    // Get the actual string value from pool
    const std::string& InternedString::getString() const
    {
        if (pool) {
            return pool->getStringById(poolId);
        }
        static const std::string empty;
        return empty;
    }
}
