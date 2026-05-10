#include "NativeRegistry.hpp"
#include <cstddef>
#include "builtin/BuiltinNatives.hpp"
#include <algorithm>

namespace environment::registry
{
    NativeRegistry::NativeRegistry() = default;
    NativeRegistry::~NativeRegistry() = default;

    void NativeRegistry::initialize()
    {
        builtin::registerAll(*this, methodCallHandler);
    }

    void NativeRegistry::cleanup()
    {
        nativeFunctions.clear();
    }

    std::string NativeRegistry::getComponentName() const
    {
        return "NativeRegistry";
    }

    void NativeRegistry::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        nativeFunctions[name] = function;
    }

    bool NativeRegistry::unregisterNativeFunction(const std::string& name)
    {
        return nativeFunctions.erase(name) > 0;
    }

    NativeFunction NativeRegistry::findNativeFunction(const std::string& name) const
    {
        auto it = nativeFunctions.find(name);
        return (it != nativeFunctions.end()) ? it->second : NativeFunction{nullptr, nullptr};
    }

    bool NativeRegistry::hasNativeFunction(const std::string& name) const
    {
        return nativeFunctions.find(name) != nativeFunctions.end();
    }

    std::vector<std::string> NativeRegistry::getAllNativeFunctionNames() const
    {
        std::vector<std::string> names;
        names.reserve(nativeFunctions.size());
        for (const auto& [name, _] : nativeFunctions)
        {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t NativeRegistry::getNativeFunctionCount() const
    {
        return nativeFunctions.size();
    }

    void NativeRegistry::setMethodCallHandler(MethodCallHandler handler)
    {
        methodCallHandler = handler;
    }
}
