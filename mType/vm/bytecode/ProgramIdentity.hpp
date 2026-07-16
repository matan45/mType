#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace vm::bytecode
{
    // Process-local identity for one in-memory BytecodeProgram instance.
    // It is deliberately not serialized: loading the same .mtc twice must
    // produce two cache domains because their metadata and instruction
    // addresses have independent lifetimes.
    struct ProgramId
    {
        uint64_t value = 0;

        constexpr bool operator==(ProgramId other) const noexcept
        {
            return value == other.value;
        }

        constexpr bool operator!=(ProgramId other) const noexcept
        {
            return !(*this == other);
        }
    };

    struct ProgramIdHash
    {
        size_t operator()(ProgramId id) const noexcept
        {
            return std::hash<uint64_t>{}(id.value);
        }
    };

    // A BytecodeProgram owns this token. Copying a program creates a new
    // cache domain; moving it transfers the domain to the destination and
    // gives the moved-from object a fresh, valid identity.
    class ProgramIdentity
    {
    public:
        ProgramIdentity() noexcept
            : id(allocate())
        {
        }

        ProgramIdentity(const ProgramIdentity&) noexcept
            : id(allocate())
        {
        }

        ProgramIdentity(ProgramIdentity&& other) noexcept
            : id(other.id)
        {
            other.id = allocate();
        }

        ProgramIdentity& operator=(const ProgramIdentity& other) noexcept
        {
            if (this != &other)
            {
                id = allocate();
            }
            return *this;
        }

        ProgramIdentity& operator=(ProgramIdentity&& other) noexcept
        {
            if (this != &other)
            {
                id = other.id;
                other.id = allocate();
            }
            return *this;
        }

        ProgramId get() const noexcept { return id; }

    private:
        inline static std::atomic<uint64_t> nextId{1};

        static ProgramId allocate() noexcept
        {
            // Zero is reserved for an invalid/default key. A 64-bit wrap is
            // not realistic, but skipping zero keeps that invariant complete.
            uint64_t value = nextId.fetch_add(1, std::memory_order_relaxed);
            if (value == 0)
            {
                value = nextId.fetch_add(1, std::memory_order_relaxed);
            }
            return ProgramId{value};
        }

        ProgramId id;
    };
}
