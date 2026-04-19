#pragma once
//
// ValueShim — MYT-126 SPIKE.
//
// Type-check functions (isInt/isFloat/.../isObject/isNativeArray/etc.) plus
// scalar accessors (asInt/asFloat/asBool/asString/asInternedString) live in
// ValueType.hpp so headers can use them without pulling in the heap-type
// definitions. This file provides the remaining heap-type accessors
// (asObject/asLambda/asNativeArray/...) — those need the complete types for
// TypedBridge template instantiation.
//
// Return types are symmetric across flags: accessors return a shared_ptr
// BY VALUE so `auto obj = value::asObject(v);` is portable.
//

#include "ValueType.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "ValueObject.hpp"
#include "NativeArray.hpp"
#include "FlatMultiArray.hpp"
#include "SparseMultiArray.hpp"
#include "PromiseValue.hpp"
#include "arrays/object/FlatMultiObjectArray.hpp"

#ifndef MTYPE_TAGGED_VALUE
#include <variant>
#include <memory>
#include <utility>
#endif

namespace value
{
#ifndef MTYPE_TAGGED_VALUE

    inline std::shared_ptr<runtimeTypes::klass::ObjectInstance> asObject(const Value& v)
    {
        return std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
    }
    inline std::shared_ptr<ValueObject> asValueObject(const Value& v)
    {
        return std::get<std::shared_ptr<ValueObject>>(v);
    }
    inline std::shared_ptr<vm::runtime::BytecodeLambda> asLambda(const Value& v)
    {
        return std::get<std::shared_ptr<vm::runtime::BytecodeLambda>>(v);
    }
    inline std::shared_ptr<NativeArray> asNativeArray(const Value& v)
    {
        return std::get<std::shared_ptr<NativeArray>>(v);
    }
    inline std::shared_ptr<FlatMultiArray> asFlatMultiArray(const Value& v)
    {
        return std::get<std::shared_ptr<FlatMultiArray>>(v);
    }
    inline std::shared_ptr<SparseMultiArray> asSparseMultiArray(const Value& v)
    {
        return std::get<std::shared_ptr<SparseMultiArray>>(v);
    }
    inline std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> asFlatMultiObjectArray(const Value& v)
    {
        return std::get<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(v);
    }
    inline std::shared_ptr<PromiseValue> asPromise(const Value& v)
    {
        return std::get<std::shared_ptr<PromiseValue>>(v);
    }

#else

    inline std::shared_ptr<runtimeTypes::klass::ObjectInstance> asObject(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::OBJECT_INSTANCE,
                                   std::shared_ptr<runtimeTypes::klass::ObjectInstance>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<ValueObject> asValueObject(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::VALUE_OBJECT, std::shared_ptr<ValueObject>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<vm::runtime::BytecodeLambda> asLambda(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::BYTECODE_LAMBDA,
                                   std::shared_ptr<vm::runtime::BytecodeLambda>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<NativeArray> asNativeArray(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::NATIVE_ARRAY, std::shared_ptr<NativeArray>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<FlatMultiArray> asFlatMultiArray(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::FLAT_MULTI_ARRAY, std::shared_ptr<FlatMultiArray>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<SparseMultiArray> asSparseMultiArray(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::SPARSE_MULTI_ARRAY, std::shared_ptr<SparseMultiArray>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> asFlatMultiObjectArray(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::FLAT_MULTI_OBJECT_ARRAY,
                                   std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline std::shared_ptr<PromiseValue> asPromise(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::PROMISE, std::shared_ptr<PromiseValue>>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }

#endif

    inline Value makeObjectValue(std::shared_ptr<runtimeTypes::klass::ObjectInstance> p)
    {
        return Value(std::move(p));
    }
    inline Value makeLambdaValue(std::shared_ptr<vm::runtime::BytecodeLambda> p)
    {
        return Value(std::move(p));
    }
}
