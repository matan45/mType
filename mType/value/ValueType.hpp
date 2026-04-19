#pragma once
#include <string>
#include <memory>
#include "StringPool.hpp"
#include "ValueBridge.hpp"

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

    //
    // 16-byte tagged Value (MYT-126 go).
    //
    // Layout: 8-byte payload + 1-byte tag + 7 bytes padding (sizeof == 16).
    // Primitives (INT/FLOAT/BOOL/VOID/NULL_TYPE) live inline in the payload.
    // Everything else routes through a RefCounted BridgeBase* — see ValueBridge.hpp.
    //
    // Heap-type constructors are declared here but defined out-of-line in
    // ValueTypeUtils.cpp so this header doesn't need to pull in the full heap
    // type definitions.
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

        // Comparison operators. Matches flag-off std::variant::operator==
        // semantics: primitives by value, STRING by content (both STD_STRING
        // and INTERNED_STRING kinds, cross-kind falls back to content), other
        // heap kinds by held shared_ptr::get() (the shared_ptr default ==).
        bool operator==(const Value& other) const noexcept
        {
            if (tag_ != other.tag_) return false;
            switch (tag_)
            {
            case ValueType::INT:       return payload_.i == other.payload_.i;
            case ValueType::FLOAT:     return payload_.d == other.payload_.d;
            case ValueType::BOOL:      return payload_.b == other.payload_.b;
            case ValueType::VOID:
            case ValueType::NULL_TYPE: return true;
            default:                   return equalsHeap(other);
            }
        }
        bool operator!=(const Value& other) const noexcept { return !(*this == other); }

    private:
        bool equalsHeap(const Value& other) const noexcept;

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
                  "Tagged Value must be 16 bytes — adjust payload/padding");
    static_assert(alignof(Value) == 8,
                  "Tagged Value must be 8-byte aligned (JIT helper assumption)");

    // Primitive shim accessors — defined inline in ValueType.hpp so that
    // headers which only include ValueType.hpp (e.g. NativeArray.hpp) can
    // use them without pulling in ValueShim.hpp (which depends on heap types).
    // Heap-type accessors (isObject / asObject / etc.) live in ValueShim.hpp.
    // Template helpers for generic code (e.g. PrimitiveArray<T>::set).
    // holdsT<T> returns whether Value holds a T; getT<T> extracts it.
    // Only primitive Ts are supported.
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
}
