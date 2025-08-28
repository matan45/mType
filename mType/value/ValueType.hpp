#pragma once
#include <variant>
#include <string>
#include <memory>

namespace runtimeTypes::klass
{
    class ObjectInstance;
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
                               nullptr_t>;
}
