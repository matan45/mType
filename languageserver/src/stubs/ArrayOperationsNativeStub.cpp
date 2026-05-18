// Minimal stub for ArrayOperationsNative - LSP doesn't need SIMD operations
#include "../../../mType/environment/registry/builtin/ArrayOperationsNative.hpp"
#include "../../../mType/environment/Environment.hpp"

namespace runtimeTypes::global {

void ArrayOperationsNative::registerAll(std::shared_ptr<environment::Environment> env) {
    // No-op for LSP - don't need SIMD array operations for parsing
}

} // namespace runtimeTypes::global
