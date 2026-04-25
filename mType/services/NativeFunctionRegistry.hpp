#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <span>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"

namespace services
{
    using NativeFunction = environment::registry::NativeFunction;

    /**
     * Service for registering native (C++) functions, classes, and members
     * Provides interface for C++ code to expose functionality to mType scripts
     */
    class NativeFunctionRegistry
    {
    private:
        std::shared_ptr<environment::Environment> environment;

    public:
        explicit NativeFunctionRegistry(std::shared_ptr<environment::Environment> env);
        ~NativeFunctionRegistry();

        // Register native functions
        void registerNativeFunction(const std::string& name, NativeFunction function);
        void registerNativeVariable(const std::string& name, const value::Value& value);

        // Register native classes and members
        void registerNativeClass(const std::string& name);
        void registerNativeMethod(const std::string& className,
                                  const std::string& methodName,
                                  NativeFunction function,
                                  bool isStatic = false);
        void registerNativeField(const std::string& className,
                                 const std::string& fieldName,
                                 const value::Value& value,
                                 bool isStatic = false);
    };
}
