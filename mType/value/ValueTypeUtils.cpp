#include "ValueTypeUtils.hpp"
#include <cstddef>
#include <new>
#include "NativeArray.hpp"
#include "FlatMultiArray.hpp"
#include "SparseMultiArray.hpp"
#include "AsyncPromiseValue.hpp"
#include "ValueObject.hpp"
#include "arrays/object/FlatMultiObjectArray.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "PromiseValue.hpp"
#include "BridgeArena.hpp"
#include <cassert>
// BytecodeLambda lives in mtype-vm; the bridge ctor below needs the complete
// type, so this include crosses the mtype-core → mtype-vm layer.
#include "../vm/runtime/context/ExecutionContext.hpp"

namespace value {

    // Tagged Value exposes its ValueType directly; Promise folds to OBJECT
    // for call-site compatibility with the pre-migration variant semantics.
    // PROMISE_INT (inline resolved-Promise<Int>) folds the same way so type
    // checks treat it identically to a heap Promise.
    ValueType ValueTypeUtils::getValueType(const Value& value) {
        auto t = value.tag();
        if (t == ValueType::PROMISE || t == ValueType::PROMISE_INT) return ValueType::OBJECT;
        return t;
    }

    // Out-of-line Value heap ctors. Each wraps the shared_ptr (or string) in
    // a TypedBridge that carries the matching BridgeKind, then stores the
    // bridge pointer in the payload. retain() bumps the refcount to 1;
    // subsequent Value copies retain again via retainIfHeap().

    // MYT-317: BridgeBase::destroy() runs the derived TypedBridge destructor
    // (virtual dispatch via ~BridgeBase()), then hands the empty memory to
    // the per-kind freelist. Snapshot kind_ first — it's invalid after the
    // destructor runs.
    void BridgeBase::destroy() noexcept
    {
        BridgeKind k = kind_;
        this->~BridgeBase();
        BridgeArena::getInstance().releaseSlot(k, this);
    }

    namespace {
        // Releases the slot if placement-new throws. Disarmed once the bridge
        // is fully constructed — from then on releaseSlot() runs via the
        // virtual BridgeBase::destroy() path when refcount hits zero.
        struct BridgeSlotGuard
        {
            void* slot;
            BridgeKind k;
            bool fromArena;
            bool armed{true};
            ~BridgeSlotGuard() noexcept
            {
                if (!armed) return;
                if (fromArena) BridgeArena::getInstance().releaseSlot(k, slot);
                else ::operator delete(slot, std::align_val_t{alignof(std::max_align_t)});
            }
        };

        template <BridgeKind K, typename Held>
        BridgeBase* makeBridge(Held h)
        {
            using Bridge = TypedBridge<K, Held>;
            void* slot = BridgeArena::getInstance().acquireSlot(K, sizeof(Bridge));
            const bool fromArena = (slot != nullptr);
            if (!slot)
            {
                slot = ::operator new(sizeof(Bridge),
                                      std::align_val_t{alignof(std::max_align_t)});
            }
            BridgeSlotGuard guard{slot, K, fromArena};
            auto* b = ::new (slot) Bridge(std::move(h));
            guard.armed = false;
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4413)  // offsetof on non-standard-layout (Value has user-defined dtor)
#endif
    size_t Value::payloadOffset() noexcept
    {
        return offsetof(Value, payload_);
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

    // MYT-317: cross-kind string equality. Operator== routes here when both
    // sides are any string form (STRING or STRING_INLINE). string_view
    // accessor folds the representation difference into a single content
    // compare. Out-of-line because asStringView dereferences the bridge for
    // the heap case.
    bool Value::stringEquals(const Value& other) const noexcept
    {
        return asStringView(*this) == asStringView(other);
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
        assert(false && "equalsHeap: unhandled BridgeKind — add a case when introducing a new kind");
        return false;
    }

}
