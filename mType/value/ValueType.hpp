#pragma once
#include <variant>
#include <string>
#include <memory>
#include "StringPool.hpp"
#ifdef MTYPE_TAGGED_VALUE
#include "IntrusivePtr.hpp"
#endif

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace vm::runtime
{
    struct BytecodeLambda;
}

namespace value
{
    class NativeArray;
    class FlatMultiArray;
    class SparseMultiArray;
    class PromiseValue;
    class ValueObject;
}

namespace mType::value::arrays
{
    class FlatMultiObjectArray;
}


namespace value
{
    // Value types that the language supports
    enum class ValueType :uint8_t
    {
        INT,
        FLOAT,
        BOOL,
        STRING,
        VOID,
        OBJECT,
        VALUE_OBJECT,
        ARRAY,
        LAMBDA,
        NULL_TYPE
    };

#ifndef MTYPE_TAGGED_VALUE
    // Runtime value that can hold different types
    using Value = std::variant<int64_t, double, bool, std::string, InternedString, std::monostate,
                               std::shared_ptr<runtimeTypes::klass::ObjectInstance>,
                               std::shared_ptr<ValueObject>,
                               std::shared_ptr<NativeArray>,
                               std::shared_ptr<FlatMultiArray>,
                               std::shared_ptr<SparseMultiArray>,
                               std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>,
                               std::shared_ptr<vm::runtime::BytecodeLambda>,
                               std::shared_ptr<PromiseValue>,
                               nullptr_t>;
#else
    //
    // MYT-126 SPIKE: 16-byte tagged Value scaffold.
    //
    // Layout: 8-byte payload + 1-byte tag + 7 bytes padding (sizeof == 16).
    // Tag is the ValueType enum above (int/float/bool/void/null supported
    // inline; heap types route through IntrusivePtr<RefCounted>).
    //
    // Only the benchmark-path primitives are scaffolded here. String and
    // heap-backed construction is intentionally NOT yet provided — those
    // require the RefCounted migration on ObjectInstance/BytecodeLambda
    // (the other 6 heap types stay on std::shared_ptr under a thin adapter
    // in the full SPIKE). See docs/spike-myt126-tagged-value.md.
    //
    class Value final
    {
    public:
        Value() noexcept : tag_(ValueType::VOID) { payload_.i = 0; }

        explicit Value(int64_t v) noexcept : tag_(ValueType::INT) { payload_.i = v; }
        explicit Value(double v) noexcept : tag_(ValueType::FLOAT) { payload_.d = v; }
        explicit Value(bool v) noexcept : tag_(ValueType::BOOL) { payload_.b = v; }
        explicit Value(std::nullptr_t) noexcept : tag_(ValueType::NULL_TYPE) { payload_.i = 0; }
        explicit Value(std::monostate) noexcept : tag_(ValueType::VOID) { payload_.i = 0; }

        // Heap-type constructors: take ownership of an IntrusivePtr that
        // the caller has already retained. Supported types must inherit
        // RefCounted. Callers go through makeObjectValue / makeLambdaValue
        // in ValueShim.hpp so the ABI stays explicit.
        Value(ValueType heapTag, RefCounted* retainedPtr) noexcept
            : tag_(heapTag)
        {
            payload_.ptr = retainedPtr;
        }

        Value(const Value& other) noexcept : tag_(other.tag_), payload_(other.payload_)
        {
            retainIfHeap();
        }

        Value(Value&& other) noexcept : tag_(other.tag_), payload_(other.payload_)
        {
            other.tag_ = ValueType::VOID;
            other.payload_.i = 0;
        }

        Value& operator=(const Value& other) noexcept
        {
            if (this != &other)
            {
                releaseIfHeap();
                tag_ = other.tag_;
                payload_ = other.payload_;
                retainIfHeap();
            }
            return *this;
        }

        Value& operator=(Value&& other) noexcept
        {
            if (this != &other)
            {
                releaseIfHeap();
                tag_ = other.tag_;
                payload_ = other.payload_;
                other.tag_ = ValueType::VOID;
                other.payload_.i = 0;
            }
            return *this;
        }

        ~Value() noexcept { releaseIfHeap(); }

        ValueType tag() const noexcept { return tag_; }

        // Raw payload accessors used by the ValueShim free functions.
        // Do not call directly — the shim owns the discipline.
        int64_t rawInt() const noexcept { return payload_.i; }
        double rawFloat() const noexcept { return payload_.d; }
        bool rawBool() const noexcept { return payload_.b; }
        RefCounted* rawPtr() const noexcept { return payload_.ptr; }

    private:
        union Payload
        {
            int64_t i;
            double d;
            bool b;
            RefCounted* ptr;
        };

        ValueType tag_;
        // 7 bytes of tail padding — compiler generates automatically to align
        // Payload to 8. Reserved for future tag-bit expansion (e.g. GC flags).
        Payload payload_;

        bool isHeapTag() const noexcept
        {
            return tag_ == ValueType::OBJECT || tag_ == ValueType::LAMBDA;
        }

        void retainIfHeap() noexcept
        {
            if (isHeapTag() && payload_.ptr) payload_.ptr->retain();
        }

        void releaseIfHeap() noexcept
        {
            if (isHeapTag() && payload_.ptr) payload_.ptr->release();
        }
    };

    static_assert(sizeof(Value) == 16,
                  "MYT-126 tagged Value must be 16 bytes — adjust payload/padding");
    static_assert(alignof(Value) == 8,
                  "MYT-126 tagged Value must be 8-byte aligned (JIT assumption)");
#endif
}
