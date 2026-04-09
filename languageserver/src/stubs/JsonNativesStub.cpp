// JsonNatives stub for language server - LSP doesn't need JSON runtime
#include "../../../mType/json/JsonNatives.hpp"

namespace json
{
    void JsonNatives::registerAll(std::shared_ptr<environment::Environment> env) {
        // No-op for LSP
    }

    void JsonNatives::cleanup() {
        // No-op for LSP
    }
} // namespace json
