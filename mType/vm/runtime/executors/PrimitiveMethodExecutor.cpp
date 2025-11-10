#include "PrimitiveMethodExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <variant>

namespace vm::runtime {

PrimitiveMethodExecutor::PrimitiveMethodExecutor(ExecutionContext& ctx)
    : context(ctx)
    , cachedIntClass_(nullptr)
    , cachedFloatClass_(nullptr)
{}

// === Helper Methods ===

int PrimitiveMethodExecutor::unboxInt(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Int object");
    }

    // Get the 'value' field from the Int object
    value::Value fieldValue = obj->getFieldValue("value");

    if (!std::holds_alternative<int>(fieldValue)) {
        throw errors::RuntimeException("Int object 'value' field is not an int");
    }

    return std::get<int>(fieldValue);
}

float PrimitiveMethodExecutor::unboxFloat(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) {
    if (!obj) {
        throw errors::RuntimeException("Cannot unbox null Float object");
    }

    // Get the 'value' field from the Float object
    value::Value fieldValue = obj->getFieldValue("value");

    if (!std::holds_alternative<float>(fieldValue)) {
        throw errors::RuntimeException("Float object 'value' field is not a float");
    }

    return std::get<float>(fieldValue);
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> PrimitiveMethodExecutor::boxInt(int value) {
    // Get Int class definition (cached)
    auto intClass = getIntClass();

    // Try to get from cache first (Phase 2 optimization)
    auto cached = value::IntegerCache::getInt(value, intClass);
    if (cached) {
        return cached;  // Cache hit!
    }

    // Cache miss or outside range - create new Int object
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
        intClass,
        std::unordered_map<std::string, std::string>{}
    );

    // Set the 'value' field
    instance->setField("value", value);

    return instance;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> PrimitiveMethodExecutor::boxFloat(float value) {
    // Get Float class definition (cached)
    auto floatClass = getFloatClass();

    // Create new Float object (no caching for floats yet)
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
        floatClass,
        std::unordered_map<std::string, std::string>{}
    );

    // Set the 'value' field
    instance->setField("value", value);

    return instance;
}

std::shared_ptr<runtimeTypes::klass::ClassDefinition> PrimitiveMethodExecutor::getIntClass() {
    // Lazy initialization with caching
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
    // Lazy initialization with caching
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
    // Stack: [receiver: Int, arg: Int]

    // 1. Pop argument
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    // 2. Pop receiver
    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    // 3. Fast arithmetic
    int result = receiver + arg;

    // 4. Box result (with caching)
    auto resultObj = boxInt(result);

    // 5. Push result
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntSub() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = receiver - arg;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntMul() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = receiver * arg;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntDiv() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    // Check for division by zero
    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Int.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = receiver / arg;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntMod() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    // Check for modulo by zero
    if (arg == 0) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero in Int.modulo()");
    }

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = receiver % arg;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntNeg() {
    // Stack: [receiver: Int]
    // Unary operation - no argument

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = -receiver;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntAbs() {
    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = (receiver < 0) ? -receiver : receiver;
    auto resultObj = boxInt(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeIntEquals() {
    // Returns primitive bool, not Bool object
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    bool result = (receiver == arg);
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeIntCompare() {
    // Returns primitive int: -1, 0, or 1
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    int arg = unboxInt(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    int receiver = unboxInt(receiverObj);

    int result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
    context.stackManager->push(result);
}

// === Float Object Method Handlers ===

void PrimitiveMethodExecutor::handleInvokeFloatAdd() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = receiver + arg;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatSub() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = receiver - arg;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatMul() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = receiver * arg;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatDiv() {
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    // Check for division by zero
    if (arg == 0.0f) {
        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Float.divide()");
    }

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = receiver / arg;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatNeg() {
    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = -receiver;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatAbs() {
    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    float result = (receiver < 0.0f) ? -receiver : receiver;
    auto resultObj = boxFloat(result);
    context.stackManager->push(resultObj);
}

void PrimitiveMethodExecutor::handleInvokeFloatEquals() {
    // Returns primitive bool
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    bool result = (receiver == arg);
    context.stackManager->push(result);
}

void PrimitiveMethodExecutor::handleInvokeFloatCompare() {
    // Returns primitive int: -1, 0, or 1
    value::Value argValue = context.stackManager->pop();
    auto argObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(argValue);
    float arg = unboxFloat(argObj);

    value::Value receiverValue = context.stackManager->pop();
    auto receiverObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
    float receiver = unboxFloat(receiverObj);

    int result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
    context.stackManager->push(result);
}

} // namespace vm::runtime
