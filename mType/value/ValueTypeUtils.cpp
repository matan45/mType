#include "ValueTypeUtils.hpp"
#include "NativeArray.hpp"
#include "FlatMultiArray.hpp"
#include "SparseMultiArray.hpp"
#include "AsyncPromiseValue.hpp"
#include "ValueObject.hpp"
#include "arrays/object/FlatMultiObjectArray.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <variant>

namespace value {

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

}
