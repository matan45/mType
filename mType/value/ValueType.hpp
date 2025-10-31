#pragma once
#include <variant>
#include <string>
#include <memory>
#include "StringPool.hpp"

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace vm::runtime
{
    struct BytecodeLambda;
}

namespace value
{
    class NativeArray;
    class FlatMultiArray;
    class SparseMultiArray;
    class PromiseValue;
}

namespace mType::value::arrays
{
    class FlatMultiObjectArray;
}


namespace value
{
    // Value types that the language supports
    enum class ValueType :uint8_t
    {
        INT,
        FLOAT,
        BOOL,
        STRING,
        VOID,
        OBJECT,
        ARRAY,
        LAMBDA,
        NULL_TYPE
    };

    // Runtime value that can hold different types
    using Value = std::variant<int, float, bool, std::string, InternedString, std::monostate,
                               std::shared_ptr<runtimeTypes::klass::ObjectInstance>,
                               std::shared_ptr<NativeArray>,
                               std::shared_ptr<FlatMultiArray>,
                               std::shared_ptr<SparseMultiArray>,
                               std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>,
                               std::shared_ptr<vm::runtime::BytecodeLambda>,
                               std::shared_ptr<PromiseValue>,
                               nullptr_t>;
}
