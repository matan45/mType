#pragma once
#include <string>
#include "TokenType.hpp"
#include "../errors/SourceLocation.hpp"

namespace token
{
    struct Token
    {
        TokenType type;
        float floatValue;
        int intValue;
        std::string stringValue;
        errors::SourceLocation location;
    };
}
