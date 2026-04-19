#pragma once
//
// ValueShim — MYT-126 SPIKE.
//
// Free-function accessor shim for value::Value that compiles identically
// under flag-off and flag-on. Call sites migrate from
//   std::holds_alternative<int64_t>(v)  →  value::isInt(v)
//   std::get<int64_t>(v)                →  value::asInt(v)
// and become layout-agnostic.
//
// Under flag-off (MTYPE_TAGGED_VALUE not defined) the shim delegates to
// std::variant. Under flag-on it reads the tagged Value's tag/payload.
//
// Scope for the SPIKE: only the accessors needed for arithmetic_tight_loop
// and object_alloc benchmarks are defined. Additional kinds (string, the
// 6 non-benchmarked heap types) stay behind flag-off-only code paths
// walled with #ifndef MTYPE_TAGGED_VALUE in their consumers.
//

#include "ValueType.hpp"

#ifndef MTYPE_TAGGED_VALUE
#include <variant>
#include <memory>
#endif

namespace value
{
#ifndef MTYPE_TAGGED_VALUE

    inline bool isInt(const Value& v) noexcept { return std::holds_alternative<int64_t>(v); }
    inline bool isFloat(const Value& v) noexcept { return std::holds_alternative<double>(v); }
    inline bool isBool(const Value& v) noexcept { return std::holds_alternative<bool>(v); }
    inline bool isVoid(const Value& v) noexcept { return std::holds_alternative<std::monostate>(v); }
    inline bool isNullType(const Value& v) noexcept { return std::holds_alternative<std::nullptr_t>(v); }

    inline bool isObject(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
    }
    inline bool isLambda(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(v);
    }

    inline int64_t asInt(const Value& v) { return std::get<int64_t>(v); }
    inline double asFloat(const Value& v) { return std::get<double>(v); }
    inline bool asBool(const Value& v) { return std::get<bool>(v); }

    inline const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& asObject(const Value& v)
    {
        return std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
    }
    inline const std::shared_ptr<vm::runtime::BytecodeLambda>& asLambda(const Value& v)
    {
        return std::get<std::shared_ptr<vm::runtime::BytecodeLambda>>(v);
    }

#else

    inline bool isInt(const Value& v) noexcept { return v.tag() == ValueType::INT; }
    inline bool isFloat(const Value& v) noexcept { return v.tag() == ValueType::FLOAT; }
    inline bool isBool(const Value& v) noexcept { return v.tag() == ValueType::BOOL; }
    inline bool isVoid(const Value& v) noexcept { return v.tag() == ValueType::VOID; }
    inline bool isNullType(const Value& v) noexcept { return v.tag() == ValueType::NULL_TYPE; }
    inline bool isObject(const Value& v) noexcept { return v.tag() == ValueType::OBJECT; }
    inline bool isLambda(const Value& v) noexcept { return v.tag() == ValueType::LAMBDA; }

    inline int64_t asInt(const Value& v) noexcept { return v.rawInt(); }
    inline double asFloat(const Value& v) noexcept { return v.rawFloat(); }
    inline bool asBool(const Value& v) noexcept { return v.rawBool(); }

    // Pointer access under flag-on returns the raw RefCounted*. The follow-up
    // migration of ObjectInstance/BytecodeLambda to RefCounted provides the
    // concrete static_cast wrappers here.

#endif
}
