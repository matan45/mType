#pragma once
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <span>
#include <string>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ValueTypeUtils.hpp"
#include "../../value/InternedString.hpp"
#include "../../value/ValueShim.hpp"
#include "../NativeContext.hpp"
#include "NativeRegistry.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../errors/TypeException.hpp"

namespace environment::registry {

    namespace binder_detail {
        template<typename T>
        struct FunctionTraits;

        // Free function pointer
        template<typename R, typename... Args>
        struct FunctionTraits<R(*)(Args...)> {
            using ReturnType = R;
            static constexpr size_t arity = sizeof...(Args);
            template<size_t I>
            using Arg = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        // Lambdas / callables — defer to operator()
        template<typename T>
        struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

        // const member function (lambda operator() is const by default)
        template<typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...) const> {
            using ReturnType = R;
            static constexpr size_t arity = sizeof...(Args);
            template<size_t I>
            using Arg = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        // Detect: first parameter is NativeContext& (auto-supplied, not consumed from args).
        // Gated on arity > 0 so we can safely refer to Arg<0>.
        template<typename Traits, bool = (Traits::arity > 0)>
        struct ContextFirst { static constexpr bool value = false; };
        template<typename Traits>
        struct ContextFirst<Traits, true> {
            static constexpr bool value =
                std::is_same_v<std::decay_t<typename Traits::template Arg<0>>,
                               ::environment::NativeContext>;
        };
    }

    /**
     * Convert mType Value <-> C++ types. On a tag mismatch, throws errors::TypeException
     * (matches the manual extractX helper pattern used by the legacy BuiltinFunction
     * classes — strictly safer than the underlying assert-based asX accessors).
     */
    template<typename T>
    struct TypeConverter {
        static T from(const value::Value& v) {
            if constexpr (std::is_same_v<T, value::Value>) {
                return v;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                if (value::isInt(v)) return value::asInt(v);
                throw errors::TypeException("expected integer argument");
            } else if constexpr (std::is_same_v<T, int>) {
                if (value::isInt(v)) return static_cast<int>(value::asInt(v));
                throw errors::TypeException("expected integer argument");
            } else if constexpr (std::is_same_v<T, double>) {
                // Numeric: accept both int and float (matches extractNumeric pattern).
                if (value::isFloat(v)) return value::asFloat(v);
                if (value::isInt(v)) return static_cast<double>(value::asInt(v));
                throw errors::TypeException("expected numeric argument");
            } else if constexpr (std::is_same_v<T, float>) {
                if (value::isFloat(v)) return static_cast<float>(value::asFloat(v));
                if (value::isInt(v)) return static_cast<float>(value::asInt(v));
                throw errors::TypeException("expected numeric argument");
            } else if constexpr (std::is_same_v<T, bool>) {
                if (value::isBool(v)) return value::asBool(v);
                throw errors::TypeException("expected boolean argument");
            } else if constexpr (std::is_same_v<T, std::string>) {
                // MYT-317: also accept STRING_INLINE. asStringView folds all
                // three forms into one branch.
                if (value::isAnyString(v)) return std::string(value::asStringView(v));
                throw errors::TypeException("expected string argument");
            } else if constexpr (std::is_same_v<T, ::value::InternedString>) {
                if (value::isInternedString(v)) return value::asInternedString(v);
                throw errors::TypeException("expected interned string argument");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                if (value::isObject(v)) return value::asObject(v);
                throw errors::TypeException("expected object argument");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<value::NativeArray>>) {
                if (value::isNativeArray(v)) return value::asNativeArray(v);
                throw errors::TypeException("expected native array argument");
            } else {
                static_assert(std::is_same_v<T, void>, "Unsupported type in NativeBinder");
                return T{};
            }
        }

        static value::Value to(T&& val) {
            return value::Value(std::forward<T>(val));
        }
    };

    /**
     * Specialization for void return.
     */
    template<>
    struct TypeConverter<void> {
        static value::Value to() {
            return value::Value(std::monostate{});
        }
    };

    /**
     * Binder that creates a NativeDelegate from a C++ function or callable.
     *
     * Three forms:
     *   1) bind(rawFn)         — exact-signature passthrough; zero heap allocation.
     *   2) bind(func)          — generic, anonymous: arity errors don't include a name.
     *   3) bind(name, func)    — generic, named: arity errors include the function name.
     *
     * For forms (2) and (3), the binder auto-detects NativeContext& as the first
     * parameter and supplies it from the dispatcher (it's not consumed from `args`).
     */
    class NativeBinder {
    public:
        // Form 1: raw passthrough — function pointer matching the native signature.
        // Stores the function pointer directly in userData; no heap allocation.
        static NativeDelegate bind(::value::Value (*fn)(::environment::NativeContext&,
                                                        std::span<const ::value::Value>)) {
            return NativeDelegate{
                reinterpret_cast<void*>(fn),
                [](void* userData, ::environment::NativeContext& ctx,
                   std::span<const ::value::Value> args) -> ::value::Value {
                    using Fn = ::value::Value(*)(::environment::NativeContext&,
                                                  std::span<const ::value::Value>);
                    return reinterpret_cast<Fn>(userData)(ctx, args);
                }
            };
        }

        // Form 2: anonymous generic.
        template<typename Func>
        static NativeDelegate bind(Func&& func) {
            return bindImpl(std::string{}, std::forward<Func>(func));
        }

        // Form 3: named generic.
        template<typename Func>
        static NativeDelegate bind(std::string name, Func&& func) {
            return bindImpl(std::move(name), std::forward<Func>(func));
        }

    private:
        template<typename Stored>
        struct BoundState {
            std::string name;
            Stored callable;
        };

        template<typename Func>
        static NativeDelegate bindImpl(std::string name, Func&& func) {
            using Traits = binder_detail::FunctionTraits<std::decay_t<Func>>;
            constexpr bool ctxFirst = binder_detail::ContextFirst<Traits>::value;
            constexpr size_t userArity = ctxFirst ? Traits::arity - 1 : Traits::arity;
            return buildDelegate<ctxFirst>(
                std::move(name), std::forward<Func>(func),
                std::make_index_sequence<userArity>{});
        }

        template<bool CtxFirst, typename Func, size_t... Is>
        static NativeDelegate buildDelegate(std::string name, Func&& func,
                                            std::index_sequence<Is...>) {
            using Traits = binder_detail::FunctionTraits<std::decay_t<Func>>;
            using Stored = std::decay_t<Func>;
            using State = BoundState<Stored>;

            // Heap-allocate the bound state (name + callable copy). Registry holds the
            // delegate for the program lifetime; freeing on shutdown is not required.
            auto* state = new State{ std::move(name), Stored(std::forward<Func>(func)) };

            return NativeDelegate{
                state,
                [](void* userData, ::environment::NativeContext& ctx,
                   std::span<const ::value::Value> args) -> ::value::Value {
                    auto* s = static_cast<State*>(userData);
                    constexpr size_t expected = sizeof...(Is);
                    if (args.size() != expected) {
                        std::string msg = s->name.empty()
                            ? std::string{}
                            : (s->name + ": ");
                        msg += "expected " + std::to_string(expected) +
                               " argument(s), got " + std::to_string(args.size());
                        throw errors::ArgumentException(msg);
                    }

                    if constexpr (CtxFirst) {
                        if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                            (s->callable)(ctx,
                                TypeConverter<std::decay_t<typename Traits::template Arg<Is + 1>>>::from(args[Is])...);
                            return TypeConverter<void>::to();
                        } else {
                            return TypeConverter<typename Traits::ReturnType>::to(
                                (s->callable)(ctx,
                                    TypeConverter<std::decay_t<typename Traits::template Arg<Is + 1>>>::from(args[Is])...)
                            );
                        }
                    } else {
                        if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                            (s->callable)(
                                TypeConverter<std::decay_t<typename Traits::template Arg<Is>>>::from(args[Is])...);
                            return TypeConverter<void>::to();
                        } else {
                            return TypeConverter<typename Traits::ReturnType>::to(
                                (s->callable)(
                                    TypeConverter<std::decay_t<typename Traits::template Arg<Is>>>::from(args[Is])...)
                            );
                        }
                    }
                }
            };
        }
    };
}
