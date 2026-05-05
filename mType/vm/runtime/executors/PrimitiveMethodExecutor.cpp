#include "PrimitiveMethodExecutor.hpp"
#include <cstdint>
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/StringPool.hpp"
#include <variant>
#include <cassert>

namespace vm::runtime {

PrimitiveMethodExecutor::PrimitiveMethodExecutor(ExecutionContext& ctx)
    : context(ctx)
    , cachedIntClass_(nullptr)
    , cachedFloatClass_(nullptr)
{}

// === Helper Methods ===

int64_t PrimitiveMethodExecutor::unboxInt(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Int object");
    }
    // Use indexed access - "value" field is always at index 0 for primitive types
    // See PrimitiveTypeTag.hpp for the compiler-enforced invariant
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
    // Use indexed access - "value" field is always at index 0 for primitive types
    // See PrimitiveTypeTag.hpp for the compiler-enforced invariant
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
        // Use indexed access - "value" field is always at index 0
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
    // Fast path: already a raw primitive (from lazy re-boxing)
    if (value::isFloat(val)) {
        return value::asFloat(val);
    }
    if (value::isValueObject(val)) {
        auto obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Float value object");
        // Use indexed access - "value" field is always at index 0
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
    // Fast path: lazy re-boxing left a raw bool.
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
    // Fast path: raw string (lazy re-boxing).
    if (value::isString(val)) {
        return value::asString(val);
    }
    if (value::isInternedString(val)) {
        return std::string(value::asInternedString(val).getString());
    }
    if (value::isValueObject(val)) {
        const auto& obj = value::asValueObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null String value object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (value::isString(fieldValue)) return value::asString(fieldValue);
        if (value::isInternedString(fieldValue))
            return std::string(value::asInternedString(fieldValue).getString());
        throw errors::RuntimeException("String value object 'value' field is not a string");
    }
    if (value::isObject(val)) {
        const auto& obj = value::asObject(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null String object");
        const value::Value& fieldValue = obj->getFieldByIndex(0);
        if (value::isString(fieldValue)) return value::asString(fieldValue);
        if (value::isInternedString(fieldValue))
            return std::string(value::asInternedString(fieldValue).getString());
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
        auto classRegistry = context.environment->getClassRegistry();
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
        auto classRegistry = context.environment->getClassRegistry();
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

// === Int Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeIntAdd() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    // Lazy re-boxing: push raw primitive, only box at escape points
    context.stackManager->push(receiver + arg);
}

void PrimitiveMethodExecutor::handleInvokeIntSub() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push(receiver - arg);
}

void PrimitiveMethodExecutor::handleInvokeIntMul() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push(receiver * arg);
}

void PrimitiveMethodExecutor::handleInvokeIntDiv() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Int.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push(receiver / arg);
}

void PrimitiveMethodExecutor::handleInvokeIntMod() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero in Int.modulo()");
    }

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push(receiver % arg);
}

void PrimitiveMethodExecutor::handleInvokeIntNeg() {
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push(-receiver);
}

void PrimitiveMethodExecutor::handleInvokeIntAbs() {
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    context.stackManager->push((receiver < 0) ? -receiver : receiver);
}

void PrimitiveMethodExecutor::handleInvokeIntEquals() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    bool result = (receiver == arg);
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeIntCompare() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeIntGetValue() {
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);
    context.stackManager->push(receiver);
}

void PrimitiveMethodExecutor::handleInvokeFloatGetValue() {
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);
    context.stackManager->push(receiver);
}

void PrimitiveMethodExecutor::handleInvokeIntLessThan() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);
    context.stackManager->push(receiver < arg);
}

void PrimitiveMethodExecutor::handleInvokeIntLessEqual() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);
    context.stackManager->push(receiver <= arg);
}

void PrimitiveMethodExecutor::handleInvokeIntGreaterThan() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);
    context.stackManager->push(receiver > arg);
}

void PrimitiveMethodExecutor::handleInvokeIntGreaterEqual() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);
    context.stackManager->push(receiver >= arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatLessThan() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);
    context.stackManager->push(receiver < arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatLessEqual() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);
    context.stackManager->push(receiver <= arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatGreaterThan() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);
    context.stackManager->push(receiver > arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatGreaterEqual() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);
    context.stackManager->push(receiver >= arg);
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

// === Float Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeFloatAdd() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    // Lazy re-boxing: push raw primitive
    context.stackManager->push(receiver + arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatSub() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    context.stackManager->push(receiver - arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatMul() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    context.stackManager->push(receiver * arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatDiv() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    if (arg == 0.0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Float.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    context.stackManager->push(receiver / arg);
}

void PrimitiveMethodExecutor::handleInvokeFloatNeg() {
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    context.stackManager->push(-receiver);
}

void PrimitiveMethodExecutor::handleInvokeFloatAbs() {
    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    context.stackManager->push((receiver < 0.0) ? -receiver : receiver);
}

void PrimitiveMethodExecutor::handleInvokeFloatEquals() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    bool result = (receiver == arg);
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeFloatCompare() {
    value::Value argValue = context.stackManager->pop();
    double arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    double receiver = unboxFloatFromValue(receiverValue);

    int64_t result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
    context.stackManager->push(result);
}

// === Bool Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeBoolAnd() {
    value::Value argValue = context.stackManager->pop();
    bool arg = unboxBoolFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    bool receiver = unboxBoolFromValue(receiverValue);
    context.stackManager->push(value::Value(receiver && arg));
}

void PrimitiveMethodExecutor::handleInvokeBoolOr() {
    value::Value argValue = context.stackManager->pop();
    bool arg = unboxBoolFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    bool receiver = unboxBoolFromValue(receiverValue);
    context.stackManager->push(value::Value(receiver || arg));
}

void PrimitiveMethodExecutor::handleInvokeBoolXor() {
    value::Value argValue = context.stackManager->pop();
    bool arg = unboxBoolFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    bool receiver = unboxBoolFromValue(receiverValue);
    // Mirror Bool.xor: (a||b) && !(a&&b), equivalent to a != b for bools.
    context.stackManager->push(value::Value(receiver != arg));
}

void PrimitiveMethodExecutor::handleInvokeBoolNot() {
    value::Value receiverValue = context.stackManager->pop();
    bool receiver = unboxBoolFromValue(receiverValue);
    context.stackManager->push(value::Value(!receiver));
}

void PrimitiveMethodExecutor::handleInvokeBoolEquals() {
    value::Value argValue = context.stackManager->pop();
    bool arg = unboxBoolFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    bool receiver = unboxBoolFromValue(receiverValue);
    context.stackManager->push(value::Value(receiver == arg));
}

// === String Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeStringLength() {
    value::Value receiverValue = context.stackManager->pop();
    std::string s = unboxStringFromValue(receiverValue);
    context.stackManager->push(static_cast<int64_t>(s.size()));
}

void PrimitiveMethodExecutor::handleInvokeStringConcat() {
    value::Value argValue = context.stackManager->pop();
    std::string arg = unboxStringFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    std::string receiver = unboxStringFromValue(receiverValue);
    std::string result = std::move(receiver) + arg;
    // Try the global StringPool first — for cycling patterns (e.g. the
    // benchmark concats "item-" + (i%13) producing 13 unique strings) every
    // hit after warmup avoids the bridge allocation. intern() returns an
    // empty handle for strings outside [1, 1024] chars; fall back to the raw
    // STD_STRING bridge in that case.
    value::InternedString interned = value::StringPool::getInstance().intern(result);
    if (!interned.empty()) {
        context.stackManager->push(value::Value(std::move(interned)));
    } else {
        context.stackManager->push(value::Value(std::move(result)));
    }
}

void PrimitiveMethodExecutor::handleInvokeStringEquals() {
    value::Value argValue = context.stackManager->pop();
    std::string arg = unboxStringFromValue(argValue);
    value::Value receiverValue = context.stackManager->pop();
    std::string receiver = unboxStringFromValue(receiverValue);
    context.stackManager->push(value::Value(receiver == arg));
}

void PrimitiveMethodExecutor::handleInvokeStringIsEmpty() {
    value::Value receiverValue = context.stackManager->pop();
    std::string s = unboxStringFromValue(receiverValue);
    context.stackManager->push(value::Value(s.empty()));
}

} // namespace vm::runtime
