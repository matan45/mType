#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

// Forward declarations
namespace runtimeTypes::klass { class ClassDefinition; }
namespace vm::bytecode { class BytecodeProgram; }

namespace vm::jit::ic
{
    constexpr size_t IC_MAX_POLYMORPHIC_ENTRIES = 4;

    enum class ICState : uint8_t
    {
        UNINITIALIZED,
        MONOMORPHIC,
        POLYMORPHIC,
        MEGAMORPHIC
    };

    // ---- Field IC ----

    struct FieldICEntry
    {
        const runtimeTypes::klass::ClassDefinition* shape = nullptr;
        size_t fieldIndex = SIZE_MAX;
    };

    struct FieldInlineCache
    {
        ICState state = ICState::UNINITIALIZED;
        uint8_t entryCount = 0;
        std::array<FieldICEntry, IC_MAX_POLYMORPHIC_ENTRIES> entries{};

        const FieldICEntry* lookup(const runtimeTypes::klass::ClassDefinition* shape) const
        {
            for (uint8_t i = 0; i < entryCount; ++i)
            {
                if (entries[i].shape == shape)
                {
                    return &entries[i];
                }
            }
            return nullptr;
        }

        bool addEntry(const runtimeTypes::klass::ClassDefinition* shape, size_t fieldIndex)
        {
            // Check for duplicate — shape already cached
            if (lookup(shape) != nullptr)
            {
                return true;  // Already present, no-op
            }

            if (entryCount >= IC_MAX_POLYMORPHIC_ENTRIES)
            {
                state = ICState::MEGAMORPHIC;
                return false;
            }

            entries[entryCount] = {shape, fieldIndex};
            ++entryCount;

            if (entryCount == 1)
            {
                state = ICState::MONOMORPHIC;
            }
            else
            {
                state = ICState::POLYMORPHIC;
            }
            return true;
        }
    };

    // ---- Method IC ----

    // MYT-184: MYT-161's cached JitEntryPtr/jitEntry field was removed along
    // with tryDirectJitMethodDispatch (see JitHelpers_Objects.cpp for the
    // rationale). Method IC hits dispatch via callMethodFromJitDirect's
    // mini-interpret loop — no cached native entry is consulted, so the
    // per-entry pointer served no purpose.

    struct MethodICEntry
    {
        const runtimeTypes::klass::ClassDefinition* shape = nullptr;
        const void* funcMetadata = nullptr;
        size_t startOffset = 0;
        // Pre-resolved qualified function name (pointer into constant pool or cached string)
        std::string qualifiedName;
        // MYT-163: receiver kind recorded at IC populate time. F-a inlining is
        // restricted to ObjectInstance receivers; ValueObject sites fall through
        // to the generic jit_call_method_ic path. The flag is still set for
        // ObjectInstance entries (as `false`) so eligibility can reject a site
        // without chasing the ClassDefinition vtable at emit time.
        bool receiverIsValueObject = false;
        // MYT-182: program that owns funcMetadata / startOffset. Library
        // callees live in loadedPrograms[programIndex]; without this, the
        // IC fast paths jump into the caller's bytecode and crash. Pointer
        // is kept for immediate use on the hot path; the index is pushed
        // onto CallFrame for restoration via ControlFlowExecutor return
        // handling.
        const bytecode::BytecodeProgram* program = nullptr;
        size_t programIndex = 0;
    };

    struct MethodInlineCache
    {
        ICState state = ICState::UNINITIALIZED;
        uint8_t entryCount = 0;
        std::array<MethodICEntry, IC_MAX_POLYMORPHIC_ENTRIES> entries{};

        const MethodICEntry* lookup(const runtimeTypes::klass::ClassDefinition* shape) const
        {
            for (uint8_t i = 0; i < entryCount; ++i)
            {
                if (entries[i].shape == shape)
                {
                    return &entries[i];
                }
            }
            return nullptr;
        }

        bool addEntry(const MethodICEntry& entry)
        {
            // Check for duplicate — shape already cached
            if (lookup(entry.shape) != nullptr)
            {
                return true;  // Already present, no-op
            }

            if (entryCount >= IC_MAX_POLYMORPHIC_ENTRIES)
            {
                state = ICState::MEGAMORPHIC;
                return false;
            }

            entries[entryCount] = entry;
            ++entryCount;

            if (entryCount == 1)
            {
                state = ICState::MONOMORPHIC;
            }
            else
            {
                state = ICState::POLYMORPHIC;
            }
            return true;
        }
    };

    // ---- Type Feedback for Arithmetic ----

    enum class ObservedType : uint8_t
    {
        NONE   = 0,
        INT    = 1 << 0,
        FLOAT  = 1 << 1,
        STRING = 1 << 2,
        OBJECT = 1 << 3,
        MIXED  = 1 << 4
    };

    inline ObservedType operator|(ObservedType a, ObservedType b)
    {
        return static_cast<ObservedType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline ObservedType operator&(ObservedType a, ObservedType b)
    {
        return static_cast<ObservedType>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    struct TypeFeedback
    {
        ObservedType leftType = ObservedType::NONE;
        ObservedType rightType = ObservedType::NONE;
        uint32_t executionCount = 0;
        bool specialized = false;
    };
}
