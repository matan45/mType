#pragma once
#include <variant>
#include <string>
#include <memory>
#include "StringPool.hpp"
#ifdef MTYPE_TAGGED_VALUE
#include "ValueBridge.hpp"
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
        NULL_TYPE,
        PROMISE
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
    // MYT-126 SPIKE: 16-byte tagged Value.
    //
    // Layout: 8-byte payload + 1-byte tag + 7 bytes padding (sizeof == 16).
    // Primitives (INT/FLOAT/BOOL/VOID/NULL_TYPE) live inline in the payload.
    // Everything else routes through a RefCounted BridgeBase* — see ValueBridge.hpp.
    //
    // Heap-type constructors are declared here but defined out-of-line in
    // ValueType.cpp so this header doesn't need to pull in the full heap type
    // definitions.
    //
    class Value final
    {
    public:
        Value() noexcept : tag_(ValueType::VOID) { payload_.i = 0; }

        Value(int64_t v) noexcept : tag_(ValueType::INT) { payload_.i = v; }
        Value(int v) noexcept : tag_(ValueType::INT) { payload_.i = v; }
        Value(unsigned int v) noexcept : tag_(ValueType::INT) { payload_.i = v; }
        Value(size_t v) noexcept : tag_(ValueType::INT) { payload_.i = static_cast<int64_t>(v); }
        Value(double v) noexcept : tag_(ValueType::FLOAT) { payload_.d = v; }
        Value(float v) noexcept : tag_(ValueType::FLOAT) { payload_.d = v; }
        Value(bool v) noexcept : tag_(ValueType::BOOL) { payload_.b = v; }
        Value(std::nullptr_t) noexcept : tag_(ValueType::NULL_TYPE) { payload_.i = 0; }
        Value(std::monostate) noexcept : tag_(ValueType::VOID) { payload_.i = 0; }

        // Implicit ctors from each shared_ptr heap type + string types.
        // Each wraps the argument in a TypedBridge<Kind, Held> and stores
        // the bridge pointer. Defined in ValueType.cpp.
        Value(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& p);
        Value(std::shared_ptr<runtimeTypes::klass::ObjectInstance>&& p);
        Value(const std::shared_ptr<ValueObject>& p);
        Value(std::shared_ptr<ValueObject>&& p);
        Value(const std::shared_ptr<NativeArray>& p);
        Value(std::shared_ptr<NativeArray>&& p);
        Value(const std::shared_ptr<FlatMultiArray>& p);
        Value(std::shared_ptr<FlatMultiArray>&& p);
        Value(const std::shared_ptr<SparseMultiArray>& p);
        Value(std::shared_ptr<SparseMultiArray>&& p);
        Value(const std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>& p);
        Value(std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>&& p);
        Value(const std::shared_ptr<vm::runtime::BytecodeLambda>& p);
        Value(std::shared_ptr<vm::runtime::BytecodeLambda>&& p);
        Value(const std::shared_ptr<PromiseValue>& p);
        Value(std::shared_ptr<PromiseValue>&& p);
        Value(const std::string& s);
        Value(std::string&& s);
        Value(const char* s);
        Value(const InternedString& s);
        Value(InternedString&& s);

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

        // Raw payload accessors — used only by ValueShim free functions.
        int64_t rawInt() const noexcept { return payload_.i; }
        double rawFloat() const noexcept { return payload_.d; }
        bool rawBool() const noexcept { return payload_.b; }
        BridgeBase* rawBridge() const noexcept { return static_cast<BridgeBase*>(payload_.ptr); }

        // Comparison operators required by tests / variant-backed call sites
        // that compare Values directly. Minimal SPIKE impl: tag equality plus
        // identity comparison for heap, payload bitwise compare for inline.
        bool operator==(const Value& other) const noexcept
        {
            if (tag_ != other.tag_) return false;
            if (isHeapTag()) return payload_.ptr == other.payload_.ptr;
            return payload_.i == other.payload_.i;
        }
        bool operator!=(const Value& other) const noexcept { return !(*this == other); }

    private:
        union Payload
        {
            int64_t i;
            double d;
            bool b;
            RefCounted* ptr;
        };

        ValueType tag_;
        // 7 bytes of tail padding — compiler inserts automatically to align
        // Payload to 8. Reserved for future tag-bit expansion (e.g. GC flags).
        Payload payload_;

        bool isHeapTag() const noexcept
        {
            switch (tag_)
            {
            case ValueType::STRING:
            case ValueType::OBJECT:
            case ValueType::VALUE_OBJECT:
            case ValueType::ARRAY:
            case ValueType::LAMBDA:
            case ValueType::PROMISE:
                return true;
            default:
                return false;
            }
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

    // Primitive shim accessors — defined inline in ValueType.hpp so that
    // headers which only include ValueType.hpp (e.g. NativeArray.hpp) can
    // use them without pulling in ValueShim.hpp (which depends on heap types).
    // Heap-type accessors (isObject / asObject / etc.) live in ValueShim.hpp.
    // Template helpers for generic code (e.g. PrimitiveArray<T>::set).
    // holdsT<T> returns whether Value holds a T; getT<T> extracts it.
    // Under flag-on these dispatch on tag; under flag-off they delegate to
    // std::variant. Only primitive Ts are supported under flag-on.
#ifdef MTYPE_TAGGED_VALUE
    template <typename T> inline bool holdsT(const Value& v) noexcept
    {
        if constexpr (std::is_same_v<T, int64_t>) return v.tag() == ValueType::INT;
        else if constexpr (std::is_same_v<T, double>) return v.tag() == ValueType::FLOAT;
        else if constexpr (std::is_same_v<T, bool>) return v.tag() == ValueType::BOOL;
        else return false;
    }
    template <typename T> inline T getT(const Value& v) noexcept
    {
        if constexpr (std::is_same_v<T, int64_t>) return static_cast<T>(v.rawInt());
        else if constexpr (std::is_same_v<T, double>) return static_cast<T>(v.rawFloat());
        else if constexpr (std::is_same_v<T, bool>) return v.rawBool();
        else return T{};
    }
#else
    template <typename T> inline bool holdsT(const Value& v) noexcept { return std::holds_alternative<T>(v); }
    template <typename T> inline T getT(const Value& v) { return std::get<T>(v); }
#endif

#ifndef MTYPE_TAGGED_VALUE
    inline bool isInt(const Value& v) noexcept { return std::holds_alternative<int64_t>(v); }
    inline bool isFloat(const Value& v) noexcept { return std::holds_alternative<double>(v); }
    inline bool isBool(const Value& v) noexcept { return std::holds_alternative<bool>(v); }
    inline bool isVoid(const Value& v) noexcept { return std::holds_alternative<std::monostate>(v); }
    inline bool isNullType(const Value& v) noexcept { return std::holds_alternative<std::nullptr_t>(v); }
    inline bool isString(const Value& v) noexcept { return std::holds_alternative<std::string>(v); }
    inline bool isInternedString(const Value& v) noexcept { return std::holds_alternative<InternedString>(v); }
    inline bool isObject(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
    }
    inline bool isValueObject(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<ValueObject>>(v);
    }
    inline bool isLambda(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(v);
    }
    inline bool isNativeArray(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<NativeArray>>(v);
    }
    inline bool isFlatMultiArray(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<FlatMultiArray>>(v);
    }
    inline bool isSparseMultiArray(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<SparseMultiArray>>(v);
    }
    inline bool isFlatMultiObjectArray(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(v);
    }
    inline bool isPromise(const Value& v) noexcept
    {
        return std::holds_alternative<std::shared_ptr<PromiseValue>>(v);
    }
    inline int64_t asInt(const Value& v) { return std::get<int64_t>(v); }
    inline double asFloat(const Value& v) { return std::get<double>(v); }
    inline bool asBool(const Value& v) { return std::get<bool>(v); }
    inline const std::string& asString(const Value& v) { return std::get<std::string>(v); }
    inline const InternedString& asInternedString(const Value& v) { return std::get<InternedString>(v); }
#else
    inline bool isInt(const Value& v) noexcept { return v.tag() == ValueType::INT; }
    inline bool isFloat(const Value& v) noexcept { return v.tag() == ValueType::FLOAT; }
    inline bool isBool(const Value& v) noexcept { return v.tag() == ValueType::BOOL; }
    inline bool isVoid(const Value& v) noexcept { return v.tag() == ValueType::VOID; }
    inline bool isNullType(const Value& v) noexcept { return v.tag() == ValueType::NULL_TYPE; }
    inline bool isObject(const Value& v) noexcept { return v.tag() == ValueType::OBJECT; }
    inline bool isValueObject(const Value& v) noexcept { return v.tag() == ValueType::VALUE_OBJECT; }
    inline bool isLambda(const Value& v) noexcept { return v.tag() == ValueType::LAMBDA; }
    inline bool isPromise(const Value& v) noexcept { return v.tag() == ValueType::PROMISE; }
    inline bool isString(const Value& v) noexcept
    {
        return v.tag() == ValueType::STRING && v.rawBridge()->kind() == BridgeKind::STD_STRING;
    }
    inline bool isInternedString(const Value& v) noexcept
    {
        return v.tag() == ValueType::STRING && v.rawBridge()->kind() == BridgeKind::INTERNED_STRING;
    }
    inline bool isNativeArray(const Value& v) noexcept
    {
        return v.tag() == ValueType::ARRAY && v.rawBridge()->kind() == BridgeKind::NATIVE_ARRAY;
    }
    inline bool isFlatMultiArray(const Value& v) noexcept
    {
        return v.tag() == ValueType::ARRAY && v.rawBridge()->kind() == BridgeKind::FLAT_MULTI_ARRAY;
    }
    inline bool isSparseMultiArray(const Value& v) noexcept
    {
        return v.tag() == ValueType::ARRAY && v.rawBridge()->kind() == BridgeKind::SPARSE_MULTI_ARRAY;
    }
    inline bool isFlatMultiObjectArray(const Value& v) noexcept
    {
        return v.tag() == ValueType::ARRAY && v.rawBridge()->kind() == BridgeKind::FLAT_MULTI_OBJECT_ARRAY;
    }
    inline int64_t asInt(const Value& v) noexcept { return v.rawInt(); }
    inline double asFloat(const Value& v) noexcept { return v.rawFloat(); }
    inline bool asBool(const Value& v) noexcept { return v.rawBool(); }
    inline const std::string& asString(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::STD_STRING, std::string>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
    inline const InternedString& asInternedString(const Value& v)
    {
        using Bridge = TypedBridge<BridgeKind::INTERNED_STRING, InternedString>;
        return static_cast<Bridge*>(v.rawBridge())->get();
    }
#endif
}
