#pragma once
#include <variant>
#include <string>
#include <memory>
#include <vector>

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace value
{
    class NativeArray;
    class FlatMultiArray;
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
        NULL_TYPE
    };

    // Runtime value that can hold different types
    using Value = std::variant<int, float, bool, std::string, std::monostate,
                               std::shared_ptr<runtimeTypes::klass::ObjectInstance>,
                               std::shared_ptr<NativeArray>,
                               std::shared_ptr<FlatMultiArray>,
                               nullptr_t>;
    
    // Helper function to get ValueType from Value
    inline ValueType getValueType(const Value& value) {
        // Explicit check for FlatMultiArray before using std::visit
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(value)) {
            return ValueType::OBJECT;
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
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                return ValueType::VOID;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                return ValueType::OBJECT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<NativeArray>>) {
                return ValueType::OBJECT; // Arrays are treated as objects
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                return ValueType::NULL_TYPE;
            } else {
                return ValueType::VOID;
            }
        }, value);
    }
}
