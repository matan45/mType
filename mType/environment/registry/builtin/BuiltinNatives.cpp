#include "BuiltinNatives.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "ValuePrinter.hpp"
#include "../NativeBinder.hpp"
#include "../../NativeContext.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../value/PromiseValue.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../runtime/EventLoop.hpp"
#include "../../../vm/runtime/VirtualMachine.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
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

    // ---- Random ----------------------------------------------------------
    static std::mt19937_64& randomEngine()
    {
        thread_local std::mt19937_64 engine{ std::random_device{}() };
        return engine;
    }

    static int64_t randomNextInt_fn(int64_t min, int64_t max)
    {
        if (max <= min)
        {
            throw errors::RuntimeException("__random_nextInt: max must be greater than min");
        }

        std::uniform_int_distribution<int64_t> distribution(min, max - 1);
        return distribution(randomEngine());
    }

    static double randomNextFloat_fn(double min, double max)
    {
        if (max <= min)
        {
            throw errors::RuntimeException("__random_nextFloat: max must be greater than min");
        }

        std::uniform_real_distribution<double> distribution(min, max);
        return distribution(randomEngine());
    }

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

    static int64_t deterministicStringHash(const std::string& str)
    {
        uint64_t hash = 14695981039346656037ull;
        for (unsigned char c : str)
        {
            hash ^= c;
            hash *= 1099511628211ull;
        }
        return static_cast<int64_t>(hash & 0x7FFFFFFFull);
    }

    static int64_t hashCode_fn(const Value& arg)
    {
        if (value::isString(arg))
        {
            return deterministicStringHash(value::asString(arg));
        }
        if (value::isInternedString(arg))
        {
            return deterministicStringHash(value::asInternedString(arg).getString());
        }
        if (value::isInt(arg))
        {
            // Identity hash, masked to 31 bits. Deterministic across stdlibs
            // (libstdc++ uses identity, MSVC mixes via FNV — they disagreed).
            uint64_t bits = static_cast<uint64_t>(value::asInt(arg));
            return static_cast<int64_t>(bits & 0x7FFFFFFFull);
        }
        if (value::isFloat(arg))
        {
            // Hash the bit pattern. Deterministic across stdlibs.
            double d = value::asFloat(arg);
            uint64_t bits;
            std::memcpy(&bits, &d, sizeof(bits));
            return static_cast<int64_t>(bits & 0x7FFFFFFFull);
        }
        if (value::isBool(arg))
        {
            return value::asBool(arg) ? static_cast<int64_t>(1231) : static_cast<int64_t>(1237);
        }
        if (value::isObject(arg))
        {
            auto obj = value::asObject(arg);
            if (!obj) return 0;
            return deterministicStringHash(obj->getContentHash());
        }
        return 0;
    }

    static Value delay_fn(::environment::NativeContext& ctx,
                          std::span<const Value> args)
    {
        if (args.size() != 1)
        {
            throw errors::RuntimeException("delay: expected 1 argument(s)");
        }
        if (!value::isInt(args[0]))
        {
            throw errors::RuntimeException("delay: ms argument must be int");
        }
        if (!ctx.vm)
        {
            throw errors::RuntimeException("delay: VM context is not available");
        }

        auto promise = std::make_shared<value::AsyncPromiseValue>();
        int delayMs = static_cast<int>(value::asInt(args[0]));
        auto* eventLoop = ctx.vm->ensureEventLoop();
        eventLoop->scheduleDelayedTask([promise]() -> value::Value
        {
            if (promise->isPending())
            {
                promise->resolve(std::monostate{});
            }
            return std::monostate{};
        }, delayMs);

        return std::static_pointer_cast<value::PromiseValue>(promise);
    }

    static std::string exceptionTypeName(const Value& exceptionValue)
    {
        if (value::isAnyObject(exceptionValue))
        {
            if (auto* obj = value::asObjectInstanceRaw(exceptionValue))
            {
                return obj->getTypeName();
            }
        }
        return "RuntimeException";
    }

    static std::string exceptionMessage(const Value& exceptionValue)
    {
        if (value::isAnyObject(exceptionValue))
        {
            if (auto* obj = value::asObjectInstanceRaw(exceptionValue))
            {
                try
                {
                    Value message = obj->getFieldValue("message");
                    if (value::isString(message)) return value::asString(message);
                    if (value::isInternedString(message)) return value::asInternedString(message).getString();
                }
                catch (...)
                {
                }
            }
        }
        return "delayed rejection";
    }

    static Value delayReject_fn(::environment::NativeContext& ctx,
                                std::span<const Value> args)
    {
        if (args.size() != 2)
        {
            throw errors::RuntimeException("delayReject: expected 2 argument(s)");
        }
        if (!value::isInt(args[0]))
        {
            throw errors::RuntimeException("delayReject: ms argument must be int");
        }
        if (!ctx.vm)
        {
            throw errors::RuntimeException("delayReject: VM context is not available");
        }

        auto promise = std::make_shared<value::AsyncPromiseValue>();
        int delayMs = static_cast<int>(value::asInt(args[0]));
        Value exceptionValue = args[1];
        std::string typeName = exceptionTypeName(exceptionValue);
        std::string message = exceptionMessage(exceptionValue);

        auto* eventLoop = ctx.vm->ensureEventLoop();
        eventLoop->scheduleDelayedTask([promise, exceptionValue, typeName, message]() -> value::Value
        {
            if (promise->isPending())
            {
                promise->rejectWithException(exceptionValue, typeName, message);
            }
            return std::monostate{};
        }, delayMs);

        return std::static_pointer_cast<value::PromiseValue>(promise);
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
        registry.registerNativeFunction("__random_nextInt", NativeBinder::bind("__random_nextInt", &randomNextInt_fn));
        registry.registerNativeFunction("__random_nextFloat", NativeBinder::bind("__random_nextFloat", &randomNextFloat_fn));

        // String manipulation.
        registry.registerNativeFunction("substring",   NativeBinder::bind("substring",   &substring_fn));
        registry.registerNativeFunction("toUpperCase", NativeBinder::bind("toUpperCase", &toUpperCase_fn));
        registry.registerNativeFunction("toLowerCase", NativeBinder::bind("toLowerCase", &toLowerCase_fn));
        registry.registerNativeFunction("indexOf",     NativeBinder::bind("indexOf",     &indexOf_fn));

        // Async timing. delay() returns a pending Promise<void> that the event
        // loop resolves after the requested delay; delayReject() returns a
        // pending Promise<void> that rejects with the supplied exception after
        // the delay. Building blocks for cooperative sleep / timeout patterns;
        // also make the executeAwait suspend branch (both resolve and reject
        // sub-paths) reachable from script code.
        registry.registerNativeFunction("delay",       NativeBinder::bind(&delay_fn));
        registry.registerNativeFunction("delayReject", NativeBinder::bind(&delayReject_fn));
    }
}
