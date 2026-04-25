#include "BuiltinNatives.hpp"
#include "ValuePrinter.hpp"
#include "../NativeBinder.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace environment::registry::builtin
{
    using ::value::Value;

    // ---- Math (single-arg numeric → double) -----------------------------
    static double sqrt_fn(double x) { return std::sqrt(x); }
    static double sin_fn(double x)  { return std::sin(x); }
    static double cos_fn(double x)  { return std::cos(x); }
    static double tan_fn(double x)  { return std::tan(x); }
    static double asin_fn(double x) { return std::asin(x); }
    static double acos_fn(double x) { return std::acos(x); }
    static double atan_fn(double x) { return std::atan(x); }

    // ---- Math (two-arg numeric) -----------------------------------------
    static double atan2_fn(double y, double x) { return std::atan2(y, x); }

    // ---- String operations ----------------------------------------------
    static int64_t strLength_fn(const std::string& s)
    {
        return static_cast<int64_t>(s.length());
    }

    static std::string toUpperCase_fn(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    static std::string toLowerCase_fn(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    static int64_t indexOf_fn(const std::string& haystack, const std::string& needle)
    {
        size_t pos = haystack.find(needle);
        if (pos == std::string::npos) return -1;
        return static_cast<int64_t>(pos);
    }

    static std::string substring_fn(const std::string& str, int64_t startIndex, int64_t length)
    {
        if (startIndex < 0)
            throw errors::RuntimeException("substring: startIndex cannot be negative");
        if (length < 0)
            throw errors::RuntimeException("substring: length cannot be negative");

        const size_t strLen = str.length();
        if (static_cast<size_t>(startIndex) > strLen)
            throw errors::RuntimeException("substring: startIndex out of bounds");

        size_t actualLength = static_cast<size_t>(length);
        if (static_cast<size_t>(startIndex) + actualLength > strLen)
            actualLength = strLen - static_cast<size_t>(startIndex);

        return str.substr(static_cast<size_t>(startIndex), actualLength);
    }

    // ---- Misc (any-Value dispatch) --------------------------------------
    static Value parsePrimitive_fn(const Value& arg)
    {
        if (value::isString(arg))         return value::asString(arg);
        if (value::isInternedString(arg)) return value::asInternedString(arg).getString();
        if (value::isInt(arg))            return std::to_string(value::asInt(arg));
        if (value::isFloat(arg))
        {
            std::string str = std::to_string(value::asFloat(arg));
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (!str.empty() && str.back() == '.') str += '0';
            return str;
        }
        if (value::isBool(arg))    return value::asBool(arg) ? std::string("true") : std::string("false");
        if (value::isVoid(arg))    return std::string("void");
        if (value::isNullType(arg)) return std::string("null");
        return std::string("unknown");
    }

    static int64_t hashCode_fn(const Value& arg)
    {
        if (value::isString(arg))
        {
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(value::asString(arg)) & 0x7FFFFFFF);
        }
        if (value::isInternedString(arg))
        {
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(value::asInternedString(arg).getString()) & 0x7FFFFFFF);
        }
        if (value::isInt(arg))
        {
            std::hash<int64_t> hasher;
            return static_cast<int64_t>(hasher(value::asInt(arg)) & 0x7FFFFFFF);
        }
        if (value::isFloat(arg))
        {
            std::hash<double> hasher;
            return static_cast<int64_t>(hasher(value::asFloat(arg)) & 0x7FFFFFFF);
        }
        if (value::isBool(arg))
        {
            return value::asBool(arg) ? static_cast<int64_t>(1231) : static_cast<int64_t>(1237);
        }
        if (value::isObject(arg))
        {
            auto obj = value::asObject(arg);
            if (!obj) return 0;
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(obj->getContentHash()) & 0x7FFFFFFF);
        }
        return 0;
    }

    void registerAll(NativeRegistry& registry, MethodCallHandler printHandler)
    {
        // Print is variadic — register a raw-form delegate manually since the
        // binder's auto-unpack only supports fixed arity.
        {
            auto printer = std::make_shared<ValuePrinter>(printHandler);
            auto* userData = new std::shared_ptr<ValuePrinter>(std::move(printer));
            auto thunk = [](void* userData,
                            ::environment::NativeContext& /*ctx*/,
                            std::span<const Value> args) -> Value
            {
                auto& p = *static_cast<std::shared_ptr<ValuePrinter>*>(userData);
                for (const auto& arg : args) p->print(arg, std::cout);
                std::cout << std::endl;
                return std::monostate{};
            };
            registry.registerNativeFunction("print", NativeDelegate{ userData, thunk });
        }

        // Misc — accept any Value, dispatch internally.
        registry.registerNativeFunction("parsePrimitive", NativeBinder::bind("parsePrimitive", &parsePrimitive_fn));
        registry.registerNativeFunction("hashCode",       NativeBinder::bind("hashCode",       &hashCode_fn));
        registry.registerNativeFunction("strLength",      NativeBinder::bind("strLength",      &strLength_fn));

        // Math.
        registry.registerNativeFunction("sqrt",  NativeBinder::bind("sqrt",  &sqrt_fn));
        registry.registerNativeFunction("sin",   NativeBinder::bind("sin",   &sin_fn));
        registry.registerNativeFunction("cos",   NativeBinder::bind("cos",   &cos_fn));
        registry.registerNativeFunction("tan",   NativeBinder::bind("tan",   &tan_fn));
        registry.registerNativeFunction("asin",  NativeBinder::bind("asin",  &asin_fn));
        registry.registerNativeFunction("acos",  NativeBinder::bind("acos",  &acos_fn));
        registry.registerNativeFunction("atan",  NativeBinder::bind("atan",  &atan_fn));
        registry.registerNativeFunction("atan2", NativeBinder::bind("atan2", &atan2_fn));

        // String manipulation.
        registry.registerNativeFunction("substring",   NativeBinder::bind("substring",   &substring_fn));
        registry.registerNativeFunction("toUpperCase", NativeBinder::bind("toUpperCase", &toUpperCase_fn));
        registry.registerNativeFunction("toLowerCase", NativeBinder::bind("toLowerCase", &toLowerCase_fn));
        registry.registerNativeFunction("indexOf",     NativeBinder::bind("indexOf",     &indexOf_fn));
    }
}
