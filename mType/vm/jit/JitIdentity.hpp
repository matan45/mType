#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "../bytecode/ProgramIdentity.hpp"

namespace vm::jit
{
    inline size_t combineIdentityHash(size_t lhs, size_t rhs) noexcept
    {
        // 64-bit golden-ratio mix; still correct on 32-bit size_t after the
        // normal narrowing conversion.
        return lhs ^ (rhs + static_cast<size_t>(0x9e3779b97f4a7c15ULL)
                      + (lhs << 6) + (lhs >> 2));
    }

    struct FunctionId
    {
        bytecode::ProgramId programId;
        std::string name;

        bool operator==(const FunctionId& other) const noexcept
        {
            return programId == other.programId && name == other.name;
        }
    };

    struct FunctionLookupId
    {
        bytecode::ProgramId programId;
        std::string_view name;
    };

    struct FunctionIdHash
    {
        using is_transparent = void;

        size_t operator()(const FunctionId& id) const noexcept
        {
            return combineIdentityHash(
                bytecode::ProgramIdHash{}(id.programId),
                // Use the same hasher as the heterogeneous lookup overload;
                // equal FunctionId/FunctionLookupId keys must hash identically.
                std::hash<std::string_view>{}(id.name));
        }

        size_t operator()(FunctionLookupId id) const noexcept
        {
            return combineIdentityHash(
                bytecode::ProgramIdHash{}(id.programId),
                std::hash<std::string_view>{}(id.name));
        }
    };

    struct FunctionIdEqual
    {
        using is_transparent = void;

        bool operator()(const FunctionId& lhs,
                        const FunctionId& rhs) const noexcept
        {
            return lhs == rhs;
        }

        bool operator()(const FunctionId& lhs,
                        FunctionLookupId rhs) const noexcept
        {
            return lhs.programId == rhs.programId && lhs.name == rhs.name;
        }

        bool operator()(FunctionLookupId lhs,
                        const FunctionId& rhs) const noexcept
        {
            return rhs.programId == lhs.programId && rhs.name == lhs.name;
        }
    };

    struct SiteId
    {
        bytecode::ProgramId programId;
        size_t instructionOffset = 0;

        bool operator==(const SiteId& other) const noexcept
        {
            return programId == other.programId
                && instructionOffset == other.instructionOffset;
        }
    };

    struct SiteIdHash
    {
        size_t operator()(const SiteId& id) const noexcept
        {
            return combineIdentityHash(
                bytecode::ProgramIdHash{}(id.programId),
                std::hash<size_t>{}(id.instructionOffset));
        }
    };

    // Reverse-inlining edges use the compact frame-name handle, but that
    // handle is only meaningful inside the program that interned it.
    struct InlineCalleeId
    {
        bytecode::ProgramId programId;
        uint32_t frameNameId = UINT32_MAX;

        bool operator==(const InlineCalleeId& other) const noexcept
        {
            return programId == other.programId
                && frameNameId == other.frameNameId;
        }
    };

    struct InlineCalleeIdHash
    {
        size_t operator()(const InlineCalleeId& id) const noexcept
        {
            return combineIdentityHash(
                bytecode::ProgramIdHash{}(id.programId),
                std::hash<uint32_t>{}(id.frameNameId));
        }
    };
}
