#include "ValueTypeUtils.hpp"
#include "NativeArray.hpp"
#include "FlatMultiArray.hpp"
#include "SparseMultiArray.hpp"
#include "AsyncPromiseValue.hpp"
#include "ValueObject.hpp"
#include "arrays/object/FlatMultiObjectArray.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#ifdef MTYPE_TAGGED_VALUE
#include "PromiseValue.hpp"
// MYT-126 flag-on: BytecodeLambda lives in mtype-vm; the bridge ctor below
// needs the complete type. The include crosses the mtype-core → mtype-vm
// layer but is confined to flag-on (SPIKE scope).
#include "../vm/runtime/context/ExecutionContext.hpp"
#endif
#include <variant>

namespace value {

#ifndef MTYPE_TAGGED_VALUE

    ValueType ValueTypeUtils::getValueType(const Value& value) {
        // Explicit check for multi-dimensional arrays before using std::visit
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(value)) {
            return ValueType::ARRAY;
        }
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(value)) {
            return ValueType::ARRAY;
        }
        if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(value)) {
            return ValueType::ARRAY;
        }
        if (std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(value)) {
            return ValueType::LAMBDA;
        }
        if (std::holds_alternative<std::shared_ptr<PromiseValue>>(value)) {
            return ValueType::OBJECT; // Promises are treated as objects
        }
        if (std::holds_alternative<std::shared_ptr<ValueObject>>(value)) {
            return ValueType::VALUE_OBJECT;
        }

        return std::visit([](const auto& v) -> ValueType {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int64_t>) {
                return ValueType::INT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, double>) {
                return ValueType::FLOAT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return ValueType::BOOL;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return ValueType::STRING;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, InternedString>) {
                return ValueType::STRING;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                return ValueType::VOID;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                return ValueType::OBJECT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<ValueObject>>) {
                return ValueType::VALUE_OBJECT;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<NativeArray>>) {
                return ValueType::ARRAY; // Arrays now have their own type
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                return ValueType::NULL_TYPE;
            } else {
                return ValueType::VOID;
            }
        }, value);
    }

#else
    //
    // MYT-126 flag-on: tagged Value exposes its ValueType directly; Promise
    // folds to OBJECT for compatibility with flag-off semantics.
    //
    ValueType ValueTypeUtils::getValueType(const Value& value) {
        auto t = value.tag();
        if (t == ValueType::PROMISE) return ValueType::OBJECT;
        return t;
    }

    // MYT-126: out-of-line Value heap ctors. Each wraps the shared_ptr (or
    // string) in a TypedBridge that carries the matching BridgeKind, then
    // stores the bridge pointer in the payload. retain() bumps the refcount
    // to 1; subsequent Value copies retain again via retainIfHeap().

    namespace {
        template <BridgeKind K, typename Held>
        BridgeBase* makeBridge(Held h)
        {
            auto* b = new TypedBridge<K, Held>(std::move(h));
            b->retain();
            return b;
        }
    }

    Value::Value(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& p)
        : tag_(ValueType::OBJECT) { payload_.ptr = makeBridge<BridgeKind::OBJECT_INSTANCE>(p); }
    Value::Value(std::shared_ptr<runtimeTypes::klass::ObjectInstance>&& p)
        : tag_(ValueType::OBJECT) { payload_.ptr = makeBridge<BridgeKind::OBJECT_INSTANCE>(std::move(p)); }

    Value::Value(const std::shared_ptr<ValueObject>& p)
        : tag_(ValueType::VALUE_OBJECT) { payload_.ptr = makeBridge<BridgeKind::VALUE_OBJECT>(p); }
    Value::Value(std::shared_ptr<ValueObject>&& p)
        : tag_(ValueType::VALUE_OBJECT) { payload_.ptr = makeBridge<BridgeKind::VALUE_OBJECT>(std::move(p)); }

    Value::Value(const std::shared_ptr<NativeArray>& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::NATIVE_ARRAY>(p); }
    Value::Value(std::shared_ptr<NativeArray>&& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::NATIVE_ARRAY>(std::move(p)); }

    Value::Value(const std::shared_ptr<FlatMultiArray>& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::FLAT_MULTI_ARRAY>(p); }
    Value::Value(std::shared_ptr<FlatMultiArray>&& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::FLAT_MULTI_ARRAY>(std::move(p)); }

    Value::Value(const std::shared_ptr<SparseMultiArray>& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::SPARSE_MULTI_ARRAY>(p); }
    Value::Value(std::shared_ptr<SparseMultiArray>&& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::SPARSE_MULTI_ARRAY>(std::move(p)); }

    Value::Value(const std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::FLAT_MULTI_OBJECT_ARRAY>(p); }
    Value::Value(std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>&& p)
        : tag_(ValueType::ARRAY) { payload_.ptr = makeBridge<BridgeKind::FLAT_MULTI_OBJECT_ARRAY>(std::move(p)); }

    Value::Value(const std::shared_ptr<vm::runtime::BytecodeLambda>& p)
        : tag_(ValueType::LAMBDA) { payload_.ptr = makeBridge<BridgeKind::BYTECODE_LAMBDA>(p); }
    Value::Value(std::shared_ptr<vm::runtime::BytecodeLambda>&& p)
        : tag_(ValueType::LAMBDA) { payload_.ptr = makeBridge<BridgeKind::BYTECODE_LAMBDA>(std::move(p)); }

    Value::Value(const std::shared_ptr<PromiseValue>& p)
        : tag_(ValueType::PROMISE) { payload_.ptr = makeBridge<BridgeKind::PROMISE>(p); }
    Value::Value(std::shared_ptr<PromiseValue>&& p)
        : tag_(ValueType::PROMISE) { payload_.ptr = makeBridge<BridgeKind::PROMISE>(std::move(p)); }

    Value::Value(const std::string& s)
        : tag_(ValueType::STRING) { payload_.ptr = makeBridge<BridgeKind::STD_STRING, std::string>(s); }
    Value::Value(std::string&& s)
        : tag_(ValueType::STRING) { payload_.ptr = makeBridge<BridgeKind::STD_STRING, std::string>(std::move(s)); }
    Value::Value(const char* s)
        : tag_(ValueType::STRING) { payload_.ptr = makeBridge<BridgeKind::STD_STRING, std::string>(std::string(s)); }

    Value::Value(const InternedString& s)
        : tag_(ValueType::STRING) { payload_.ptr = makeBridge<BridgeKind::INTERNED_STRING, InternedString>(s); }
    Value::Value(InternedString&& s)
        : tag_(ValueType::STRING) { payload_.ptr = makeBridge<BridgeKind::INTERNED_STRING, InternedString>(std::move(s)); }

    // MYT-189: out-of-line heap-content equality. Mirrors the implicit
    // std::variant::operator== dispatch used under flag-off — strings by
    // content, other heap kinds by the held shared_ptr's underlying raw
    // pointer (same as shared_ptr::operator== semantics).
    namespace {
        const std::string& stringAt(const Value& v)
        {
            auto kind = v.rawBridge()->kind();
            if (kind == BridgeKind::STD_STRING)
            {
                using Bridge = TypedBridge<BridgeKind::STD_STRING, std::string>;
                return static_cast<Bridge*>(v.rawBridge())->get();
            }
            using Bridge = TypedBridge<BridgeKind::INTERNED_STRING, InternedString>;
            return static_cast<Bridge*>(v.rawBridge())->get().getString();
        }

        template <BridgeKind K, typename Held>
        const void* heldRaw(const Value& v)
        {
            return static_cast<TypedBridge<K, Held>*>(v.rawBridge())->get().get();
        }
    }

    bool Value::equalsHeap(const Value& other) const noexcept
    {
        BridgeBase* lhs = rawBridge();
        BridgeBase* rhs = other.rawBridge();
        if (!lhs || !rhs) return lhs == rhs;

        if (tag_ == ValueType::STRING)
        {
            return stringAt(*this) == stringAt(other);
        }

        if (lhs->kind() != rhs->kind()) return false;

        switch (lhs->kind())
        {
        case BridgeKind::OBJECT_INSTANCE:
            return heldRaw<BridgeKind::OBJECT_INSTANCE,
                           std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*this)
                == heldRaw<BridgeKind::OBJECT_INSTANCE,
                           std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(other);
        case BridgeKind::VALUE_OBJECT:
            return heldRaw<BridgeKind::VALUE_OBJECT, std::shared_ptr<ValueObject>>(*this)
                == heldRaw<BridgeKind::VALUE_OBJECT, std::shared_ptr<ValueObject>>(other);
        case BridgeKind::BYTECODE_LAMBDA:
            return heldRaw<BridgeKind::BYTECODE_LAMBDA,
                           std::shared_ptr<vm::runtime::BytecodeLambda>>(*this)
                == heldRaw<BridgeKind::BYTECODE_LAMBDA,
                           std::shared_ptr<vm::runtime::BytecodeLambda>>(other);
        case BridgeKind::NATIVE_ARRAY:
            return heldRaw<BridgeKind::NATIVE_ARRAY, std::shared_ptr<NativeArray>>(*this)
                == heldRaw<BridgeKind::NATIVE_ARRAY, std::shared_ptr<NativeArray>>(other);
        case BridgeKind::FLAT_MULTI_ARRAY:
            return heldRaw<BridgeKind::FLAT_MULTI_ARRAY, std::shared_ptr<FlatMultiArray>>(*this)
                == heldRaw<BridgeKind::FLAT_MULTI_ARRAY, std::shared_ptr<FlatMultiArray>>(other);
        case BridgeKind::SPARSE_MULTI_ARRAY:
            return heldRaw<BridgeKind::SPARSE_MULTI_ARRAY,
                           std::shared_ptr<SparseMultiArray>>(*this)
                == heldRaw<BridgeKind::SPARSE_MULTI_ARRAY,
                           std::shared_ptr<SparseMultiArray>>(other);
        case BridgeKind::FLAT_MULTI_OBJECT_ARRAY:
            return heldRaw<BridgeKind::FLAT_MULTI_OBJECT_ARRAY,
                           std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(*this)
                == heldRaw<BridgeKind::FLAT_MULTI_OBJECT_ARRAY,
                           std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(other);
        case BridgeKind::PROMISE:
            return heldRaw<BridgeKind::PROMISE, std::shared_ptr<PromiseValue>>(*this)
                == heldRaw<BridgeKind::PROMISE, std::shared_ptr<PromiseValue>>(other);
        case BridgeKind::STD_STRING:
        case BridgeKind::INTERNED_STRING:
            // Handled by the tag_==STRING branch above.
            return false;
        }
        return false;
    }

#endif

}
