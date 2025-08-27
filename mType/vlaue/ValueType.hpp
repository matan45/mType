#pragma once
#include <variant>
#include <string>
#include <memory>

namespace value
{
    // Value types that the language supports
    enum class ValueType
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
                               std::shared_ptr<runtime::ObjectInstance>,
                               nullptr_t>;
}
