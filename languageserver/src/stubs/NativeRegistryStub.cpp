// Minimal stub for NativeRegistry - LSP doesn't need runtime builtin functions
#include "../../../mType/environment/registry/NativeRegistry.hpp"

namespace environment::registry {

NativeRegistry::NativeRegistry() {
    // No builtins needed for LSP
}

NativeRegistry::~NativeRegistry() {
    // No-op for LSP
}

void NativeRegistry::initialize() {
    // No-op for LSP - don't register builtin functions
}

void NativeRegistry::cleanup() {
    // No-op for LSP
}

std::string NativeRegistry::getComponentName() const {
    return "NativeRegistry (LSP Stub)";
}

void NativeRegistry::registerNativeFunction(const std::string& name, NativeFunction function) {
    nativeFunctions[name] = function;
}

NativeFunction NativeRegistry::findNativeFunction(const std::string& name) const {
    auto it = nativeFunctions.find(name);
    if (it != nativeFunctions.end()) {
        return it->second;
    }
    return NativeFunction{nullptr, nullptr};
}

bool NativeRegistry::hasNativeFunction(const std::string& name) const {
    return nativeFunctions.count(name) > 0;
}

std::vector<std::string> NativeRegistry::getAllNativeFunctionNames() const {
    std::vector<std::string> names;
    names.reserve(nativeFunctions.size());
    for (const auto& pair : nativeFunctions) {
        names.push_back(pair.first);
    }
    return names;
}

size_t NativeRegistry::getNativeFunctionCount() const {
    return nativeFunctions.size();
}

void NativeRegistry::setMethodCallHandler(MethodCallHandler handler) {
    methodCallHandler = handler;
}

} // namespace environment::registry
