#include "NativeRegistry.hpp"
#include <algorithm>
#include <iostream>

namespace environment::registry
{
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

    void NativeRegistry::registerBuiltinFunctions()
    {
        registerNativeFunction("print", [](const std::vector<Value>& args) -> Value {
            for (const auto& arg : args)
            {
                std::visit([](const auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                        std::cout << (value ? "true" : "false");
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>)
                        std::cout << "void";
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
                        std::cout << "null";
                }, arg);
            }
            std::cout << std::endl;
            return std::monostate{};
        });

        registerNativeFunction("println", [](const std::vector<Value>& args) -> Value {
            for (const auto& arg : args)
            {
                std::visit([](const auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                        std::cout << (value ? "true" : "false");
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>)
                        std::cout << "void";
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
                        std::cout << "null";
                }, arg);
            }
            std::cout << std::endl;
            return std::monostate{};
        });

        registerNativeFunction("toString", [](const std::vector<Value>& args) -> Value {
            if (args.size() != 1) {
                throw std::runtime_error("toString expects exactly 1 argument");
            }
            
            return std::visit([](const auto& value) -> Value {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
                    return value; // Already a string
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>) {
                    return std::to_string(value);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>) {
                    return std::to_string(value);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>) {
                    return value ? std::string("true") : std::string("false");
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>) {
                    return std::string("void");
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>) {
                    return std::string("null");
                }
                else {
                    return std::string("unknown");
                }
            }, args[0]);
        });
    }
}
