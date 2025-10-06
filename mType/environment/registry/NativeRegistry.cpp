#include "NativeRegistry.hpp"
#include <algorithm>
#include <iostream>
#include <functional>
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../value/StringPool.hpp"

namespace environment::registry
{
    namespace {
        // Helper function to get string representation from an object's toString() method
        std::optional<std::string> getObjectStringRepresentation(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& value,
            const std::function<Value(std::shared_ptr<runtimeTypes::klass::ObjectInstance>, const std::string&, const std::vector<Value>&)>& methodCallHandler)
        {
            if (!value || !methodCallHandler) {
                return std::nullopt;
            }

            auto classDef = value->getClassDefinition();
            auto toStringMethod = classDef ? classDef->findMethod("toString", 0) : nullptr;

            // No toString method or it's static - return nullopt
            if (!toStringMethod || toStringMethod->isStatic()) {
                return std::nullopt;
            }

            try {
                Value result = methodCallHandler(value, "toString", {});

                // Handle string result
                if (std::holds_alternative<std::string>(result)) {
                    return std::get<std::string>(result);
                }

                // Handle InternedString result
                if (std::holds_alternative<value::InternedString>(result)) {
                    return std::get<value::InternedString>(result).getString();
                }

                // Handle object result with value field
                if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result)) {
                    auto resultObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result);
                    if (resultObj) {
                        Value fieldValue = resultObj->getFieldValue("value");

                        if (std::holds_alternative<std::string>(fieldValue)) {
                            return std::get<std::string>(fieldValue);
                        }

                        if (std::holds_alternative<value::InternedString>(fieldValue)) {
                            return std::get<value::InternedString>(fieldValue).getString();
                        }
                    }
                }
            }
            catch (...) {
                // If toString() fails, return nullopt to fall back to default representation
            }

            return std::nullopt;
        }
    }

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
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
                        std::cout << value.getString();
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                        std::cout << value;
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                        std::cout << (value ? "true" : "false");
                    else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>)
                        std::cout << "null";
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
                            // Try to get string representation from toString() method
                            auto stringRep = getObjectStringRepresentation(value, methodCallHandler);
                            if (stringRep.has_value()) {
                                std::cout << stringRep.value();
                            } else {
                                // Default object representation
                                std::cout << "[object " << value->getTypeName() << "]";
                            }
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
                throw errors::ArgumentException("parsePrimitive expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    return value; // Already a string
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
                {
                    return value.getString(); // Convert InternedString to string
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
                {
                    return std::to_string(value);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
                {
                    // Format float with proper precision, removing trailing zeros
                    std::string str = std::to_string(value);
                    // Remove trailing zeros
                    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                    // Remove trailing decimal point if no fractional part
                    if (str.back() == '.') {
                        str += '0'; // Keep at least one decimal place (e.g., "3.0" not "3")
                    }
                    return str;
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
                throw errors::ArgumentException("str::length expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    return static_cast<int>(value.length());
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
                {
                    return static_cast<int>(value.getString().length());
                }
                else
                {
                    throw errors::RuntimeException("str::length can only be called on strings");
                }
            }, args[0]);
        });

        // Add hashCode function for generating hash codes for any value
        registerNativeFunction("hashCode", [this](const std::vector<Value>& args) -> Value
        {
            if (args.size() != 1)
            {
                throw errors::ArgumentException("hashCode expects exactly 1 argument");
            }

            return std::visit([](const auto& value) -> Value
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
                {
                    // Hash string content
                    std::hash<std::string> hasher;
                    return static_cast<int>(hasher(value) & 0x7FFFFFFF); // Keep positive
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
                {
                    // Hash InternedString content
                    std::hash<std::string> hasher;
                    return static_cast<int>(hasher(value.getString()) & 0x7FFFFFFF); // Keep positive
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
