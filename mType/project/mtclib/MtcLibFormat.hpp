#pragma once
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace project::mtclib
{
    // Symbol kinds for export/import tables
    enum class SymbolKind : uint8_t
    {
        CLASS = 0,
        INTERFACE = 1,
        FUNCTION = 2,
        VARIABLE = 3
    };

    // Visibility levels
    enum class SymbolVisibility : uint8_t
    {
        PUBLIC = 0,
        PROTECTED = 1
    };

    struct MtcLibHeader
    {
        char magic[6] = {'M', 'T', 'C', 'L', 'I', 'B'};
        uint16_t version = 1;
        uint32_t flags = 0;   // bit 0: has_debug_info, bit 1: has_native_deps

        static constexpr uint16_t CURRENT_VERSION = 1;
        static constexpr uint32_t FLAG_HAS_DEBUG_INFO = 0x01;
        static constexpr uint32_t FLAG_HAS_NATIVE_DEPS = 0x02;
    };

    struct MtcLibMetadata
    {
        std::string name;
        std::string version;
        std::string mtypeVersion;
        uint64_t sourceHash = 0;
    };

    struct MtcLibDependency
    {
        std::string name;
        std::string versionConstraint;
        uint64_t contentHash = 0;
    };

    struct MtcLibExport
    {
        SymbolKind kind = SymbolKind::CLASS;
        std::string name;
        std::string signature;
        uint64_t bytecodeOffset = 0;  // 0xFFFFFFFFFFFFFFFF for interfaces (no bytecode)
        SymbolVisibility visibility = SymbolVisibility::PUBLIC;
    };

    struct MtcLibImport
    {
        SymbolKind kind = SymbolKind::CLASS;
        std::string name;
        std::string signature;
        std::string sourceLib;
    };

    struct MtcLibProgram
    {
        MtcLibHeader header;
        MtcLibMetadata metadata;
        std::vector<MtcLibDependency> dependencies;
        std::vector<MtcLibExport> exports;
        std::vector<MtcLibImport> imports;
        vm::bytecode::BytecodeProgram bytecodeProgram;
    };
}
