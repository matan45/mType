// mtclib stubs for language server - LSP doesn't load .mtcLib programs at runtime,
// so the loader/native registration paths are skipped. Only the symbols actually
// referenced from EnvironmentBuilder.cpp are stubbed (constructor + registerAll).
#include "../../../mType/project/mtclib/TransitiveDependencyLoader.hpp"
#include "../../../mType/project/mtclib/LibraryNatives.hpp"

namespace project::mtclib
{
    // Mirrors the real ctor's pathResolver(projectRoot) init so member
    // construction stays valid; body intentionally empty (no library load).
    TransitiveDependencyLoader::TransitiveDependencyLoader(const std::string& projectRoot)
        : pathResolver(projectRoot)
    {
    }

    void LibraryNatives::registerAll(std::shared_ptr<environment::Environment> env,
                                     std::shared_ptr<TransitiveDependencyLoader> loader) {
        // No-op for LSP
    }

    void LibraryNatives::cleanup() {
        // No-op for LSP
    }

} // namespace project::mtclib
