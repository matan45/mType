#pragma once
#include <cstdint>
#include <string>

namespace value
{
    // Fast type tag for primitive wrapper classes (Int, Float, Bool, String).
    // Used to replace string-based type checks in hot paths.
    //
    // COMPILER-ENFORCED INVARIANT:
    // Primitive wrapper classes (Int, Float, Bool, String) must declare their
    // wrapped primitive as the FIRST instance field named "value". This ensures
    // that getFieldByIndex(0) always yields the underlying primitive.
    // PrimitiveMethodExecutor relies on this for O(1) unboxing.
    //
    // NOTE ON STRING: STRING is included for unboxing purposes only (used by
    // ArithmeticExecutor and ComparisonExecutor to auto-unbox String wrapper
    // objects). Lazy re-boxing does NOT apply to String — there are no
    // INVOKE_STRING_* opcodes, and autoBoxPrimitive does not handle std::string.
    // Raw std::string values never appear where a String wrapper object is expected.
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
