// ReflectionNatives stub for language server - LSP doesn't need reflection runtime
#include "../../../mType/reflection/ReflectionNatives.hpp"

namespace reflection
{
    void ReflectionNatives::registerAll(std::shared_ptr<environment::Environment> env) {
        // No-op for LSP - reflection natives are not needed for language analysis
    }

    void ReflectionNatives::cleanup() {
        // No-op for LSP
    }

} // namespace reflection
