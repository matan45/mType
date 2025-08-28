#pragma once
#include "../IEnvironmentComponent.hpp"
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include "../../value/ValueType.hpp"

namespace environment::registry
{
    using namespace value;
    
    using NativeFunction = std::function<Value(const std::vector<Value>&)>;

    class NativeRegistry : public IEnvironmentComponent
    {
    private:
        std::unordered_map<std::string, NativeFunction> nativeFunctions;

    public:
        NativeRegistry() = default;
        ~NativeRegistry() override = default;

        void initialize() override;
        void cleanup() override;
        std::string getComponentName() const override;

        void registerNativeFunction(const std::string& name, NativeFunction function);
        NativeFunction findNativeFunction(const std::string& name) const;
        bool hasNativeFunction(const std::string& name) const;
        std::vector<std::string> getAllNativeFunctionNames() const;
        size_t getNativeFunctionCount() const;

    private:
        void registerBuiltinFunctions();
    };
}

