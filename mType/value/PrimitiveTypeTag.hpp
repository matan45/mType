#pragma once
#include <cstdint>
#include <string>

namespace value
{
    enum class PrimitiveTypeTag : uint8_t
    {
        NONE = 0,
        INT,
        FLOAT,
        BOOL,
        STRING
    };

    inline PrimitiveTypeTag classNameToPrimitiveTag(const std::string& className)
    {
        if (className == "Int") return PrimitiveTypeTag::INT;
        if (className == "Float") return PrimitiveTypeTag::FLOAT;
        if (className == "Bool") return PrimitiveTypeTag::BOOL;
        if (className == "String") return PrimitiveTypeTag::STRING;
        return PrimitiveTypeTag::NONE;
    }
}
