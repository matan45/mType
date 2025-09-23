#include "NativeRegistry.hpp"
#include <algorithm>
#include <iostream>
#include <functional>
#include "../../runtimeTypes/klass/ObjectInstance.hpp"

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

    void NativeRegistry::setMethodCallHandler(MethodCallHandler handler)
    {
        methodCallHandler = handler;
    }

    void NativeRegistry::registerBuiltinFunctions()
    {
        registerNativeFunction("print", [this](const std::vector<Value>& args) -> Value
        {
            for (const auto& arg : args)
            {
                std::visit([this](const auto& value)
                {
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
                    else if constexpr (std::is_same_v<
                        std::decay_t<decltype(value)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
                    {
                        if (!value)
                        {
                            std::cout << "null";
                        }
                        else
                        {
                            // Try to call toString() method if it exists
                            if (methodCallHandler)
                            {
                                auto classDef = value->getClassDefinition();
                                if (classDef && classDef->hasMethod("toString"))
                                {
                                    auto toStringMethod = classDef->findMethod("toString", 0);
                                    if (toStringMethod && !toStringMethod->isStatic())
                                    {
                                        try
                                        {
                                            Value result = methodCallHandler(value, "toString", {});
                                            if (std::holds_alternative<std::string>(result))
                                            {
                                                std::cout << std::get<std::string>(result);
                                                return;
                                            }
                                        }
                                        catch (...)
                                        {
                                            // If toString() fails, fall back to default representation
                                        }
                                    }
                                }
                            }
                            // Default object representation
                            std::cout << "[object " << value->getTypeName() << "]";
                        }
                    }
                }, arg);
            }
            std::cout << std::endl;
            return std::monostate{};
        });

        registerNativeFunction("parsePrimitive", [](const std::vector<Value>& args) -> Value
        {
            if (args.size() != 1)
            {
                throw std::runtime_error("parsePrimitive expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    return value; // Already a string
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                {
                    return std::to_string(value);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                {
                    return std::to_string(value);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                {
                    return value ? std::string("true") : std::string("false");
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>)
                {
                    return std::string("void");
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
                {
                    return std::string("null");
                }
                else
                {
                    return std::string("unknown");
                }
            }, args[0]);
        });

        registerNativeFunction("strLength", [](const std::vector<Value>& args) -> Value
        {
            if (args.size() != 1)
            {
                throw std::runtime_error("str::length expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    return static_cast<int>(value.length());
                }
                else
                {
                    throw std::runtime_error("str::length can only be called on strings");
                }
            }, args[0]);
        });

        // Add hashCode function for generating hash codes for any value
        registerNativeFunction("hashCode", [this](const std::vector<Value>& args) -> Value
        {
            if (args.size() != 1)
            {
                throw std::runtime_error("hashCode expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    // Hash string content
                    std::hash<std::string> hasher;
                    return static_cast<int>(hasher(value) & 0x7FFFFFFF); // Keep positive
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                {
                    // Hash int value
                    std::hash<int> hasher;
                    return static_cast<int>(hasher(value) & 0x7FFFFFFF);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                {
                    // Hash float value
                    std::hash<float> hasher;
                    return static_cast<int>(hasher(value) & 0x7FFFFFFF);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                {
                    // Hash bool value
                    return value ? 1231 : 1237; // Standard Java hash codes for true/false
                }
                else if constexpr (std::is_same_v<
                    std::decay_t<decltype(value)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
                {
                    if (!value)
                    {
                        return 0; // Null object hash
                    }

                    // Use content-based hashing for objects
                    std::string contentHash = value->getContentHash();
                    std::hash<std::string> hasher;
                    return static_cast<int>(hasher(contentHash) & 0x7FFFFFFF);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
                {
                    return 0; // Null hash code
                }
                else
                {
                    return 0; // Default hash code for unknown types
                }
            }, args[0]);
        });
    }
}
