#pragma once

#include <cstddef>
#include <cstdint>
#include "../../value/ValueShim.hpp"

namespace vm::jit::detail
{
    // Layout constants for the donation-reset in emitInlineLocalCopy. Under
    // the 16-byte tagged Value (MYT-126), tag_ is the first member of
    // value::Value so the tag byte sits at offset 0. Writing ValueType::VOID
    // into that byte donates the slot without a refcount bump — isHeapTag is
    // false for VOID, so the matching emitInlineLocalDestroy skips the release.
    struct InlineShapeLayout
    {
        size_t  tagOffset;
        uint8_t voidTag;
    };
    static_assert(sizeof(value::ValueType) == 1,
                  "tag byte layout assumes single-byte ValueType");

    inline const InlineShapeLayout& getInlineShapeLayout()
    {
        static constexpr InlineShapeLayout layout{
            /*tagOffset=*/0,
            static_cast<uint8_t>(value::ValueType::VOID)
        };
        return layout;
    }
}
