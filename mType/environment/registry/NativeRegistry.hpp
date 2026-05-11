#pragma once
#include <functional>
#include <cstddef>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <span>
#include "NativeDelegate.hpp"

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace environment::registry
{
    using NativeFunction = NativeDelegate;
    using MethodCallHandler = std::function<value::Value(std::shared_ptr<runtimeTypes::klass::ObjectInstance>, const std::string&, const std::vector<value::Value>&)>;

    class NativeRegistry
    {
    private:
        std::unordered_map<std::string, NativeFunction> nativeFunctions;
        MethodCallHandler methodCallHandler;

    public:
        explicit NativeRegistry();
        ~NativeRegistry();

        void initialize();
        void cleanup();
        std::string getComponentName() const;

        void registerNativeFunction(const std::string& name, NativeFunction function);
        bool unregisterNativeFunction(const std::string& name);
        NativeFunction findNativeFunction(const std::string& name) const;
        bool hasNativeFunction(const std::string& name) const;
        std::vector<std::string> getAllNativeFunctionNames() const;
        size_t getNativeFunctionCount() const;

        void setMethodCallHandler(MethodCallHandler handler);
    };
}
