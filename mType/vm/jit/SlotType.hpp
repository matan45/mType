#pragma once
#include <cstdint>

namespace vm::jit
{
    /**
     * Compile-time type of an operand stack slot.
     * Used to determine boxing/unboxing and which native instructions to emit.
     */
    enum class SlotType : uint8_t
    {
        INT,    // int64_t in GP register
        FLOAT,  // double in XMM register (64-bit)
        BOOL,   // bool stored as int64_t (0 or 1)
        STRING,       // InternedString in boxed stack
        OBJECT,       // shared_ptr<ObjectInstance> in boxed stack
        ARRAY,        // shared_ptr<NativeArray> in boxed stack
        VALUE_OBJECT, // shared_ptr<ValueObject> in boxed stack
        BOXED         // Unknown non-primitive Value in boxed stack
    };

    inline bool isBoxedSlotType(SlotType t)
    {
        return t == SlotType::STRING || t == SlotType::OBJECT ||
               t == SlotType::ARRAY || t == SlotType::VALUE_OBJECT ||
               t == SlotType::BOXED;
    }
}
