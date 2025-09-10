#pragma once
#include <variant>
#include <string>
#include <memory>

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace runtimeTypes::collections
{
    class Collection;
    class Array;
    class Map;
    class Set;
    class Queue;
    class Stack;
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
        NULL_TYPE,
        ARRAY,
        MAP,
        SET,
        QUEUE,
        STACK
    };

    // Runtime value that can hold different types
    using Value = std::variant<int, float, bool, std::string, std::monostate,
                               std::shared_ptr<runtimeTypes::klass::ObjectInstance>,
                               nullptr_t,
                               std::shared_ptr<runtimeTypes::collections::Array>,
                               std::shared_ptr<runtimeTypes::collections::Map>,
                               std::shared_ptr<runtimeTypes::collections::Set>,
                               std::shared_ptr<runtimeTypes::collections::Queue>,
                               std::shared_ptr<runtimeTypes::collections::Stack>>;
    
    // Helper function to get ValueType from Value
    inline ValueType getValueType(const Value& value) {
        return std::visit([](const auto& v) -> ValueType {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
                return ValueType::INT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>) {
                return ValueType::FLOAT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return ValueType::BOOL;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return ValueType::STRING;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                return ValueType::VOID;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                return ValueType::OBJECT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                return ValueType::NULL_TYPE;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Array>>) {
                return ValueType::ARRAY;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Map>>) {
                return ValueType::MAP;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Set>>) {
                return ValueType::SET;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Queue>>) {
                return ValueType::QUEUE;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Stack>>) {
                return ValueType::STACK;
            } else {
                return ValueType::VOID;
            }
        }, value);
    }
}
