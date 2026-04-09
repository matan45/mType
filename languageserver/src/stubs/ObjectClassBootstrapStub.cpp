// ObjectClassBootstrap stub for language server
#include "../../../mType/environment/registry/builtin/ObjectClassBootstrap.hpp"
#include "../../../mType/environment/Environment.hpp"

namespace environment::registry::builtin
{
    void registerObjectClass(const std::shared_ptr<environment::Environment>& environment) {
        // No-op for LSP - Object class registration not needed for analysis
    }
}
