// PluginNatives stub for language server - LSP doesn't need to load plugins
#include "../../../mType/plugin/PluginNatives.hpp"

namespace plugin
{
    void PluginNatives::registerAll(std::shared_ptr<environment::Environment> /*env*/) {
        // No-op for LSP
    }

    void PluginNatives::cleanup() {
        // No-op for LSP
    }
} // namespace plugin
