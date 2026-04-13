#include "MtcLibBuilder.hpp"
#include "../../types/TypeSignature.hpp"

namespace project::mtclib
{
    MtcLibProgram MtcLibBuilder::build(
        const vm::bytecode::BytecodeProgram& program,
        const ProjectConfig& config,
        const environment::registry::ExportRegistry& exportRegistry)
    {
        MtcLibProgram lib;

        // Header with defaults (magic = MTCLIB, version = 1, flags = 0)
        lib.header = MtcLibHeader{};

        // Metadata from project config
        lib.metadata = buildMetadata(config);

        // Dependencies from project config
        lib.dependencies = buildDependencies(config);

        // Exports from export registry + bytecode metadata
        lib.exports = buildExports(program, exportRegistry);

        // Imports are currently empty — will be populated when we support
        // cross-library symbol references (MYT-72+)
        lib.imports = {};

        // Embed the full bytecode program
        lib.bytecodeProgram = program;

        return lib;
    }

    MtcLibMetadata MtcLibBuilder::buildMetadata(const ProjectConfig& config)
    {
        MtcLibMetadata metadata;
        metadata.name = config.name;
        metadata.version = config.version;
        metadata.mtypeVersion = "0.2.0";  // Current mType version
        metadata.sourceHash = 0;          // TODO: compute hash from source files
        return metadata;
    }

    std::vector<MtcLibDependency> MtcLibBuilder::buildDependencies(const ProjectConfig& config)
    {
        std::vector<MtcLibDependency> deps;
        for (const auto& pkg : config.dependencies.packages) {
            MtcLibDependency dep;
            dep.name = pkg.name;
            dep.versionConstraint = pkg.versionRange;
            dep.contentHash = 0;  // TODO: compute from dependency content
            deps.push_back(dep);
        }
        return deps;
    }

    std::vector<MtcLibExport> MtcLibBuilder::buildExports(
        const vm::bytecode::BytecodeProgram& program,
        const environment::registry::ExportRegistry& exportRegistry)
    {
        std::vector<MtcLibExport> exports;

        // Export classes
        for (const auto& classMeta : program.getClasses()) {
            MtcLibExport exp;
            exp.kind = SymbolKind::CLASS;
            exp.name = classMeta.name;
            exp.signature = ::types::TypeSignature::encodeClassSignature(classMeta);
            exp.bytecodeOffset = 0;  // Classes don't have a single bytecode offset
            exp.visibility = SymbolVisibility::PUBLIC;
            exports.push_back(exp);
        }

        // Export interfaces
        for (const auto& ifaceMeta : program.getInterfaces()) {
            MtcLibExport exp;
            exp.kind = SymbolKind::INTERFACE;
            exp.name = ifaceMeta.name;
            exp.signature = ::types::TypeSignature::encodeInterfaceSignature(ifaceMeta);
            exp.bytecodeOffset = UINT64_MAX;  // Interfaces have no bytecode
            exp.visibility = SymbolVisibility::PUBLIC;
            exports.push_back(exp);
        }

        // Export functions
        for (const auto& [name, funcMeta] : program.getFunctions()) {
            // Skip internal/compiler-generated functions
            if (name.find("__") == 0) continue;

            MtcLibExport exp;
            exp.kind = SymbolKind::FUNCTION;
            exp.name = name;
            exp.signature = ::types::TypeSignature::encodeFunctionSignature(funcMeta);
            exp.bytecodeOffset = funcMeta.startOffset;
            exp.visibility = SymbolVisibility::PUBLIC;
            exports.push_back(exp);
        }

        return exports;
    }

    SymbolKind MtcLibBuilder::toSymbolKind(environment::registry::ExportedSymbolType type)
    {
        switch (type) {
            case environment::registry::ExportedSymbolType::CLASS:     return SymbolKind::CLASS;
            case environment::registry::ExportedSymbolType::INTERFACE: return SymbolKind::INTERFACE;
            case environment::registry::ExportedSymbolType::FUNCTION:  return SymbolKind::FUNCTION;
            case environment::registry::ExportedSymbolType::VARIABLE:  return SymbolKind::VARIABLE;
            default: return SymbolKind::CLASS;
        }
    }
}
