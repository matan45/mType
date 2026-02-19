#include "PrimitiveMethodExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ValueObject.hpp"
#include <variant>

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
    value::Value fieldValue = obj->getFieldValue("value");
    if (!std::holds_alternative<int64_t>(fieldValue)) {
        throw errors::RuntimeException("Int object 'value' field is not an int");
    }
    return std::get<int64_t>(fieldValue);
}

float PrimitiveMethodExecutor::unboxFloat(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Float object");
    }
    value::Value fieldValue = obj->getFieldValue("value");
    if (!std::holds_alternative<float>(fieldValue)) {
        throw errors::RuntimeException("Float object 'value' field is not a float");
    }
    return std::get<float>(fieldValue);
}

int64_t PrimitiveMethodExecutor::unboxIntFromValue(const value::Value& val) {
    if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
        return unboxInt(std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val));
    }
    if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
        auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Int value object");
        value::Value fieldValue = obj->getFieldValue("value");
        if (!std::holds_alternative<int64_t>(fieldValue)) {
            throw errors::RuntimeException("Int value object 'value' field is not an int");
        }
        return std::get<int64_t>(fieldValue);
    }
    if (std::holds_alternative<int64_t>(val)) {
        return std::get<int64_t>(val);
    }
    throw errors::RuntimeException("Cannot unbox Int: unexpected value type");
}

float PrimitiveMethodExecutor::unboxFloatFromValue(const value::Value& val) {
    if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
        return unboxFloat(std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val));
    }
    if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
        auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
        if (!obj) throw errors::RuntimeException("Cannot unbox null Float value object");
        value::Value fieldValue = obj->getFieldValue("value");
        if (!std::holds_alternative<float>(fieldValue)) {
            throw errors::RuntimeException("Float value object 'value' field is not a float");
        }
        return std::get<float>(fieldValue);
    }
    if (std::holds_alternative<float>(val)) {
        return std::get<float>(val);
    }
    throw errors::RuntimeException("Cannot unbox Float: unexpected value type");
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

value::Value PrimitiveMethodExecutor::boxFloatValue(float val) {
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
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
        intClass, std::unordered_map<std::string, std::string>{});
    instance->setField("value", value);
    return instance;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> PrimitiveMethodExecutor::boxFloat(float value) {
    auto floatClass = getFloatClass();
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
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

    int64_t result = receiver + arg;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntSub() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = receiver - arg;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntMul() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = receiver * arg;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntDiv() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Int.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = receiver / arg;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntMod() {
    value::Value argValue = context.stackManager->pop();
    int64_t arg = unboxIntFromValue(argValue);

    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero in Int.modulo()");
    }

    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = receiver % arg;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntNeg() {
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = -receiver;
    context.stackManager->push(boxIntValue(result));
}

void PrimitiveMethodExecutor::handleInvokeIntAbs() {
    value::Value receiverValue = context.stackManager->pop();
    int64_t receiver = unboxIntFromValue(receiverValue);

    int64_t result = (receiver < 0) ? -receiver : receiver;
    context.stackManager->push(boxIntValue(result));
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

// === Float Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeFloatAdd() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = receiver + arg;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatSub() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = receiver - arg;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatMul() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = receiver * arg;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatDiv() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    if (arg == 0.0f) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Float.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = receiver / arg;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatNeg() {
    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = -receiver;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatAbs() {
    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    float result = (receiver < 0.0f) ? -receiver : receiver;
    context.stackManager->push(boxFloatValue(result));
}

void PrimitiveMethodExecutor::handleInvokeFloatEquals() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    bool result = (receiver == arg);
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeFloatCompare() {
    value::Value argValue = context.stackManager->pop();
    float arg = unboxFloatFromValue(argValue);

    value::Value receiverValue = context.stackManager->pop();
    float receiver = unboxFloatFromValue(receiverValue);

    int64_t result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
    context.stackManager->push(result);
}

} // namespace vm::runtime
