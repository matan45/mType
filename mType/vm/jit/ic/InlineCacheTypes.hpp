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

    struct MethodICEntry
    {
        const runtimeTypes::klass::ClassDefinition* shape = nullptr;
        const void* funcMetadata = nullptr;
        size_t startOffset = 0;
        // Pre-resolved qualified function name (pointer into constant pool or cached string)
        std::string qualifiedName;
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
