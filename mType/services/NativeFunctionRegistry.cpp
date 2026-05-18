#include "NativeFunctionRegistry.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/VariableDefinition.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../value/ValueTypeUtils.hpp"

namespace services
{
    NativeFunctionRegistry::NativeFunctionRegistry(std::shared_ptr<environment::Environment> env)
        : environment(env)
    {
    }

    NativeFunctionRegistry::~NativeFunctionRegistry() = default;

    void NativeFunctionRegistry::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(name, function);
    }

    void NativeFunctionRegistry::registerNativeVariable(const std::string& name, const value::Value& value)
    {
        // Create a variable definition and register it
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            name, value::ValueTypeUtils::getValueType(value), false, false);
        varDef->setValue(value);

        environment->declareVariable(name, varDef);
    }

    void NativeFunctionRegistry::registerNativeClass(const std::string& name)
    {
        // Create a basic class definition for native classes
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(name);

        environment->registerClass(name, classDef);
    }

    void NativeFunctionRegistry::registerNativeMethod(const std::string& className,
                                                       const std::string& methodName,
                                                       NativeFunction function,
                                                       bool isStatic)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(methodName, function);
    }

    void NativeFunctionRegistry::registerNativeField(const std::string& className,
                                                      const std::string& fieldName,
                                                      const value::Value& value,
                                                      bool isStatic)
    {
        registerNativeVariable(fieldName, value);
    }
}
