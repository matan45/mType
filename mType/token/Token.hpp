#pragma once
#include <string>
#include "TokenType.hpp"
#include "../errors/SourceLocation.hpp"
#include "../value/StringPool.hpp"

namespace token
{
    struct Token
    {
        TokenType type;
        float floatValue;
        int64_t intValue;
        value::InternedString stringValue;
        errors::SourceLocation location;
    };
}
