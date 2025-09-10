#pragma once

#include <string>
#include <memory>
#include "../../value/ValueType.hpp"

namespace runtimeTypes::collections
{
    using namespace value;

    // Base interface for all collections
    class Collection
    {
    public:
        virtual ~Collection() = default;
        
        // Common collection operations
        virtual size_t size() const = 0;
        virtual bool empty() const = 0;
        virtual void clear() = 0;
        
        // Type information
        virtual std::string getTypeName() const = 0;
        virtual ValueType getElementType() const = 0;
        
        // Iterator support for for-each loops
        virtual bool hasNext() const = 0;
        virtual Value getNext() = 0;
        virtual void resetIterator() = 0;
    };
}