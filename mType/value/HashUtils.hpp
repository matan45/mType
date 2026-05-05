#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace value::hashutils
{
    // Single source of truth for the runtime hashCode() protocol.
    //
    // BuiltinNatives::hashCode_fn, JsonDeserializer::hashPrimitive, the IC and
    // JIT primitive fast-leaves (computePrimitiveProtocolHash), and
    // SpecializedCollectionStorage::rawHash MUST all route through these
    // helpers so the wrapper/native/IC/JIT/storage paths stay byte-identical.
    // The Protocol Fast Path Drift integration test detects divergence; making
    // the helpers shared here removes the way they could drift.
    //
    // - intHash:    splitmix64 finalizer over the 64-bit signed value
    // - floatHash:  splitmix64 over the double's bit pattern
    // - boolHash:   1231 / 1237 (Java compatibility, already deterministic)
    // - stringHash: FNV-1a 64-bit
    //
    // All return values are masked to 31 bits so the result fits a positive
    // mType `int` and so the bucket-index formula `hash & (capacity - 1)`
    // (HashMap.mt / HashSet.mt) lands in [0, capacity).

    inline int64_t mix64(uint64_t x) noexcept
    {
        // splitmix64 finalizer. Deterministic across stdlibs/platforms,
        // strong avalanche, ~10 cycles. Replaces both std::hash<int64_t>
        // (was identity on libstdc++/libc++ — bad distribution for keys
        // aligned to capacity) and MSVC's std::hash mixing (also unstable
        // across STL versions).
        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<int64_t>(x & 0x7FFFFFFFULL);
    }

    inline int64_t intHash(int64_t v) noexcept
    {
        return mix64(static_cast<uint64_t>(v));
    }

    inline int64_t floatHash(double d) noexcept
    {
        uint64_t bits;
        std::memcpy(&bits, &d, sizeof(bits));
        return mix64(bits);
    }

    inline int64_t boolHash(bool b) noexcept
    {
        return b ? 1231 : 1237;
    }

    inline int64_t stringHash(std::string_view s) noexcept
    {
        // FNV-1a 64-bit, masked to 31 bits.
        uint64_t h = 0xcbf29ce484222325ULL;
        for (unsigned char c : s)
        {
            h ^= c;
            h *= 0x100000001b3ULL;
        }
        return static_cast<int64_t>(h & 0x7FFFFFFFULL);
    }
}
