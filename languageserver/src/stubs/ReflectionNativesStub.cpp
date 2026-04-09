// ReflectionNatives stub for language server - LSP doesn't need reflection runtime
#include "../../../mType/reflection/ReflectionNatives.hpp"

namespace reflection
{
    // Static member definition
    std::shared_ptr<environment::Environment> ReflectionNatives::currentEnvironment = nullptr;
    std::shared_ptr<vm::runtime::VirtualMachine> ReflectionNatives::currentVM = nullptr;

    void ReflectionNatives::registerAll(std::shared_ptr<environment::Environment> env) {
        // No-op for LSP - reflection natives are not needed for language analysis
    }

    void ReflectionNatives::setEnvironment(std::shared_ptr<environment::Environment> env) {
        // No-op for LSP
    }

    void ReflectionNatives::setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm) {
        // No-op for LSP
    }

    void ReflectionNatives::cleanup() {
        // No-op for LSP
    }

} // namespace reflection
