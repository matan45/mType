// NetNatives stub for language server - LSP doesn't need socket/HTTP runtime
#include "../../../mType/net/NetNatives.hpp"

namespace net
{
    void NetNatives::registerAll(std::shared_ptr<environment::Environment> env) {
        // No-op for LSP
    }

    void NetNatives::cleanup() {
        // No-op for LSP
    }

} // namespace net
