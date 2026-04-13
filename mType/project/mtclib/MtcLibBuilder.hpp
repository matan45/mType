#pragma once
#include "MtcLibFormat.hpp"
#include "../ProjectConfig.hpp"
#include "../../environment/registry/ExportRegistry.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"

namespace project::mtclib
{
    /**
     * Builds an MtcLibProgram from compilation results.
     * Collects exports, dependencies, and metadata to package
     * alongside the compiled bytecode into a .mtcLib binary.
     */
    class MtcLibBuilder
    {
    public:
        static MtcLibProgram build(
            const vm::bytecode::BytecodeProgram& program,
            const ProjectConfig& config,
            const environment::registry::ExportRegistry& exportRegistry);

    private:
        static MtcLibMetadata buildMetadata(const ProjectConfig& config);
        static std::vector<MtcLibDependency> buildDependencies(const ProjectConfig& config);
        static std::vector<MtcLibExport> buildExports(
            const vm::bytecode::BytecodeProgram& program,
            const environment::registry::ExportRegistry& exportRegistry);
        static SymbolKind toSymbolKind(environment::registry::ExportedSymbolType type);
    };
}
