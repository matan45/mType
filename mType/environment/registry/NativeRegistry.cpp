#include "NativeRegistry.hpp"
#include "builtin/PrintFunction.hpp"
#include "builtin/ParsePrimitiveFunction.hpp"
#include "builtin/StrLengthFunction.hpp"
#include "builtin/HashCodeFunction.hpp"
#include "builtin/SqrtFunction.hpp"
#include "builtin/SinFunction.hpp"
#include "builtin/CosFunction.hpp"
#include "builtin/TanFunction.hpp"
#include "builtin/AsinFunction.hpp"
#include "builtin/AcosFunction.hpp"
#include "builtin/AtanFunction.hpp"
#include "builtin/Atan2Function.hpp"
#include "builtin/SubstringFunction.hpp"
#include "builtin/ToUpperCaseFunction.hpp"
#include "builtin/ToLowerCaseFunction.hpp"
#include "builtin/IndexOfFunction.hpp"
#include <algorithm>

namespace environment::registry
{
    // Constructor and destructor defined here where complete type of BuiltinFunction is available
    NativeRegistry::NativeRegistry() = default;
    NativeRegistry::~NativeRegistry() = default;

    void NativeRegistry::initialize()
    {
        registerBuiltinFunctions();
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

    NativeFunction NativeRegistry::findNativeFunction(const std::string& name) const
    {
        auto it = nativeFunctions.find(name);
        return (it != nativeFunctions.end()) ? it->second : nullptr;
    }

    bool NativeRegistry::hasNativeFunction(const std::string& name) const
    {
        return nativeFunctions.find(name) != nativeFunctions.end();
    }

    std::vector<std::string> NativeRegistry::getAllNativeFunctionNames() const
    {
        std::vector<std::string> names;
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

    void NativeRegistry::registerBuiltinFunctions()
    {
        // Register all builtin functions using clean, separated classes
        registerBuiltinFunction(std::make_unique<builtin::PrintFunction>(methodCallHandler));
        registerBuiltinFunction(std::make_unique<builtin::ParsePrimitiveFunction>());
        registerBuiltinFunction(std::make_unique<builtin::StrLengthFunction>());
        registerBuiltinFunction(std::make_unique<builtin::HashCodeFunction>());

        // Math functions
        registerBuiltinFunction(std::make_unique<builtin::SqrtFunction>());
        registerBuiltinFunction(std::make_unique<builtin::SinFunction>());
        registerBuiltinFunction(std::make_unique<builtin::CosFunction>());
        registerBuiltinFunction(std::make_unique<builtin::TanFunction>());
        registerBuiltinFunction(std::make_unique<builtin::AsinFunction>());
        registerBuiltinFunction(std::make_unique<builtin::AcosFunction>());
        registerBuiltinFunction(std::make_unique<builtin::AtanFunction>());
        registerBuiltinFunction(std::make_unique<builtin::Atan2Function>());

        // String manipulation functions
        registerBuiltinFunction(std::make_unique<builtin::SubstringFunction>());
        registerBuiltinFunction(std::make_unique<builtin::ToUpperCaseFunction>());
        registerBuiltinFunction(std::make_unique<builtin::ToLowerCaseFunction>());
        registerBuiltinFunction(std::make_unique<builtin::IndexOfFunction>());
    }

    void NativeRegistry::registerBuiltinFunction(std::unique_ptr<builtin::BuiltinFunction> function)
    {
        auto name = function->getName();
        auto rawPtr = function.get();
        builtinFunctions.push_back(std::move(function));

        registerNativeFunction(name, [rawPtr](const std::vector<Value>& args) -> Value
        {
            return rawPtr->execute(args);
        });
    }
}
