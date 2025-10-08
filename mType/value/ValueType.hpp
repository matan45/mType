#pragma once
#include <variant>
#include <string>
#include <memory>
#include <vector>
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
    class LambdaValue;
    class PromiseValue;
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
                               std::shared_ptr<LambdaValue>,
                               std::shared_ptr<vm::runtime::BytecodeLambda>,
                               std::shared_ptr<PromiseValue>,
                               nullptr_t>;
    
    // Helper function to get ValueType from Value
    inline ValueType getValueType(const Value& value) {
        // Explicit check for multi-dimensional arrays before using std::visit
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(value)) {
            return ValueType::ARRAY;
        }
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(value)) {
            return ValueType::ARRAY;
        }
        if (std::holds_alternative<std::shared_ptr<LambdaValue>>(value)) {
            return ValueType::LAMBDA;
        }
        if (std::holds_alternative<std::shared_ptr<PromiseValue>>(value)) {
            return ValueType::OBJECT; // Promises are treated as objects
        }

        return std::visit([](const auto& v) -> ValueType {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
                return ValueType::INT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>) {
                return ValueType::FLOAT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return ValueType::BOOL;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return ValueType::STRING;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, InternedString>) {
                return ValueType::STRING;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                return ValueType::VOID;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                return ValueType::OBJECT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<NativeArray>>) {
                return ValueType::ARRAY; // Arrays now have their own type
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                return ValueType::NULL_TYPE;
            } else {
                return ValueType::VOID;
            }
        }, value);
    }
}
