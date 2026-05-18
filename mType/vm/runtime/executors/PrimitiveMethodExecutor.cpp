#include "PrimitiveMethodExecutor.hpp"
#include <cstdint>
#include <cassert>
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/StringPool.hpp"

namespace vm::runtime {

// MYT-319: all 31 INVOKE_* handlers live in the header. The unbox/box
// helpers and the two remaining out-of-line handlers (handleInvokeBoolGetValue,
// handleInvokeStringConcat) stay here because they pull in ValueObject,
// ObjectInstancePool, StringPool, and ClassDefinition full types.

int64_t PrimitiveMethodExecutor::unboxInt(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Int object");
    }
    obj->ensureFieldVector();
    assert(obj->getClassDefinition()->getFieldIndex("value") == 0
        && "Int class must have 'value' as first field (index 0)");
    const value::Value& fieldValue = obj->getFieldByIndex(0);
    if (!value::isInt(fieldValue)) {
        throw errors::RuntimeException("Int object 'value' field is not an int");
    }
    return value::asInt(fieldValue);
}

double PrimitiveMethodExecutor::unboxFloat(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Float object");
    }
    obj->ensureFieldVector();
    assert(obj->getClassDefinition()->getFieldIndex("value") == 0
        && "Float class must have 'value' as first field (index 0)");
    const value::Value& fieldValue = obj->getFieldByIndex(0);
    if (!value::isFloat(fieldValue)) {
        throw errors::RuntimeException("Float object 'value' field is not a float");
    }
    return value::asFloat(fieldValue);
}

int64_t PrimitiveMethodExecutor::unboxIntFromValue(const value::Value& val) {
    // Fast path: already a raw primitive (from lazy re-boxing)
    if (value::isInt(val)) {
        return value::asInt(val);
    }
    if (value::isValueObject(val)) {
        auto obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Int value object");
        assert(obj->getClassDefinition()->getFieldIndex("value") == 0
            && "Int value class must have 'value' as first field (index 0)");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isInt(fieldValue)) {
            throw errors::RuntimeException("Int value object 'value' field is not an int");
        }
        return value::asInt(fieldValue);
    }
    if (value::isObject(val)) {
        return unboxInt(value::asObject(val));
    }
    throw errors::RuntimeException("Cannot unbox Int: unexpected value type");
}

double PrimitiveMethodExecutor::unboxFloatFromValue(const value::Value& val) {
    if (value::isFloat(val)) {
        return value::asFloat(val);
    }
    if (value::isValueObject(val)) {
        auto obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Float value object");
        assert(obj->getClassDefinition()->getFieldIndex("value") == 0
            && "Float value class must have 'value' as first field (index 0)");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isFloat(fieldValue)) {
            throw errors::RuntimeException("Float value object 'value' field is not a float");
        }
        return value::asFloat(fieldValue);
    }
    if (value::isObject(val)) {
        return unboxFloat(value::asObject(val));
    }
    throw errors::RuntimeException("Cannot unbox Float: unexpected value type");
}

bool PrimitiveMethodExecutor::unboxBoolFromValue(const value::Value& val) {
    if (value::isBool(val)) {
        return value::asBool(val);
    }
    if (value::isValueObject(val)) {
        const auto& obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Bool value object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isBool(fieldValue)) {
            throw errors::RuntimeException("Bool value object 'value' field is not a bool");
        }
        return value::asBool(fieldValue);
    }
    if (value::isObject(val)) {
        const auto& obj = value::asObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Bool object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isBool(fieldValue)) {
            throw errors::RuntimeException("Bool object 'value' field is not a bool");
        }
        return value::asBool(fieldValue);
    }
    throw errors::RuntimeException("Cannot unbox Bool: unexpected value type");
}

std::string PrimitiveMethodExecutor::unboxStringFromValue(const value::Value& val) {
    if (value::isString(val)) {
        return value::asString(val);
    }
    if (value::isInternedString(val)) {
        return std::string(value::asInternedString(val).getString());
    }
    // MYT-317: SSO. Construct from the inline bytes directly.
    if (value::isInlineString(val)) {
        return std::string(val.rawInlineChars(), val.rawInlineLen());
    }
    if (value::isValueObject(val)) {
        const auto& obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null String value object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        // MYT-317: SSO-aware nested string read.
        if (value::isAnyString(fieldValue)) return std::string(value::asStringView(fieldValue));
        throw errors::RuntimeException("String value object 'value' field is not a string");
    }
    if (value::isObject(val)) {
        const auto& obj = value::asObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null String object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (value::isAnyString(fieldValue)) return std::string(value::asStringView(fieldValue));
        throw errors::RuntimeException("String object 'value' field is not a string");
    }
    throw errors::RuntimeException("Cannot unbox String: unexpected value type");
}

value::Value PrimitiveMethodExecutor::boxIntValue(int64_t val) {
    auto intClass = getIntClass();
    if (intClass->isValueClass()) {
        auto valueObj = std::make_shared<value::ValueObject>(intClass);
        valueObj->setField("value", val);
        return value::Value(valueObj);
    }
    return value::Value(boxInt(val));
}

value::Value PrimitiveMethodExecutor::boxFloatValue(double val) {
    auto floatClass = getFloatClass();
    if (floatClass->isValueClass()) {
        auto valueObj = std::make_shared<value::ValueObject>(floatClass);
        valueObj->setField("value", val);
        return value::Value(valueObj);
    }
    return value::Value(boxFloat(val));
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> PrimitiveMethodExecutor::boxInt(int64_t value) {
    auto intClass = getIntClass();
    auto cached = value::IntegerCache::getInt(value, intClass);
    if (cached) {
        return cached;
    }
    auto instance = value::ObjectInstancePool::getInstance().acquire(
        intClass, std::unordered_map<std::string, std::string>{});
    instance->setField("value", value);
    return instance;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> PrimitiveMethodExecutor::boxFloat(double value) {
    auto floatClass = getFloatClass();
    auto instance = value::ObjectInstancePool::getInstance().acquire(
        floatClass, std::unordered_map<std::string, std::string>{});
    instance->setField("value", value);
    return instance;
}

std::shared_ptr<runtimeTypes::klass::ClassDefinition> PrimitiveMethodExecutor::getIntClass() {
    if (!cachedIntClass_) {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available for Int boxing");
        }
        cachedIntClass_ = classRegistry->findClass("Int");
        if (!cachedIntClass_) {
            throw errors::RuntimeException("Int class not found - ensure lib/primitives/Int.mt is loaded");
        }
    }
    return cachedIntClass_;
}

std::shared_ptr<runtimeTypes::klass::ClassDefinition> PrimitiveMethodExecutor::getFloatClass() {
    if (!cachedFloatClass_) {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available for Float boxing");
        }
        cachedFloatClass_ = classRegistry->findClass("Float");
        if (!cachedFloatClass_) {
            throw errors::RuntimeException("Float class not found - ensure lib/primitives/Float.mt is loaded");
        }
    }
    return cachedFloatClass_;
}

void PrimitiveMethodExecutor::handleInvokeBoolGetValue() {
    value::Value receiverValue = context.stackManager->pop();
    // Bool's value field holds either a raw bool variant (lazy re-boxing)
    // or a ValueObject/ObjectInstance whose field 0 holds the bool.
    if (value::isBool(receiverValue)) {
        context.stackManager->push(value::asBool(receiverValue));
        return;
    }
    if (value::isValueObject(receiverValue)) {
        const auto& obj = value::asValueObject(receiverValue);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Bool value object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isBool(fieldValue))
            throw errors::RuntimeException("Bool value object 'value' field is not a bool");
        context.stackManager->push(value::asBool(fieldValue));
        return;
    }
    if (value::isObject(receiverValue)) {
        const auto& obj = value::asObject(receiverValue);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Bool object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (!value::isBool(fieldValue))
            throw errors::RuntimeException("Bool object 'value' field is not a bool");
        context.stackManager->push(value::asBool(fieldValue));
        return;
    }
    throw errors::RuntimeException("Cannot unbox Bool: unexpected value type");
}

void PrimitiveMethodExecutor::handleInvokeStringConcat() {
    value::Value argValue = context.stackManager->pop();
    value::Value receiverValue = context.stackManager->pop();
    // MYT-317: SSO fast path. If both operands carry string content (heap or
    // inline) and the concatenated length fits in the inline buffer, build
    // the result directly in a stack-local Value with no heap allocation
    // and no StringPool lookup. Falls through to the original intern path
    // for anything else (longer results, unboxed Object/ValueObject wrappers).
    if (value::isAnyString(receiverValue) && value::isAnyString(argValue)) {
        std::string_view r = value::asStringView(receiverValue);
        std::string_view a = value::asStringView(argValue);
        const size_t total = r.size() + a.size();
        if (total <= value::Value::INLINE_STRING_CAP) {
            char buf[value::Value::INLINE_STRING_CAP];
            if (!r.empty()) std::memcpy(buf, r.data(), r.size());
            if (!a.empty()) std::memcpy(buf + r.size(), a.data(), a.size());
            context.stackManager->push(value::Value(
                value::Value::InlineStringTag{}, buf, static_cast<uint8_t>(total)));
            return;
        }
    }
    std::string arg = unboxStringFromValue(argValue);
    std::string receiver = unboxStringFromValue(receiverValue);
    std::string result = std::move(receiver) + arg;
    // Try the global StringPool first.
    value::InternedString interned = value::StringPool::getInstance().intern(result);
    if (!interned.empty()) {
        context.stackManager->push(value::Value(std::move(interned)));
    } else {
        context.stackManager->push(value::Value(std::move(result)));
    }
}

} // namespace vm::runtime
