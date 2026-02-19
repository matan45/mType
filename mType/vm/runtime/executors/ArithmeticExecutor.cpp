#include "ArithmeticExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueObject.hpp"
#include <sstream>

namespace vm::runtime
{
    ArithmeticExecutor::ArithmeticExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void ArithmeticExecutor::handleAdd() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::ADD));
    }

    void ArithmeticExecutor::handleSub() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::SUB));
    }

    void ArithmeticExecutor::handleMul() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::MUL));
    }

    void ArithmeticExecutor::handleDiv() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::DIV));
    }

    void ArithmeticExecutor::handleMod() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::MOD));
    }

    void ArithmeticExecutor::handleNeg() {
        value::Value val = context.stackManager->pop();
        if (std::holds_alternative<int64_t>(val)) {
            context.stackManager->push(-std::get<int64_t>(val));
        } else if (std::holds_alternative<float>(val)) {
            context.stackManager->push(-std::get<float>(val));
        } else {
            throw errors::RuntimeException("NEG requires numeric value");
        }
    }

    void ArithmeticExecutor::handleInc() {
        value::Value val = context.stackManager->pop();
        if (std::holds_alternative<int64_t>(val)) {
            context.stackManager->push(std::get<int64_t>(val) + 1);
        } else if (std::holds_alternative<float>(val)) {
            context.stackManager->push(std::get<float>(val) + 1.0f);
        } else {
            throw errors::RuntimeException("INC requires numeric value");
        }
    }

    void ArithmeticExecutor::handleDec() {
        value::Value val = context.stackManager->pop();
        if (std::holds_alternative<int64_t>(val)) {
            context.stackManager->push(std::get<int64_t>(val) - 1);
        } else if (std::holds_alternative<float>(val)) {
            context.stackManager->push(std::get<float>(val) - 1.0f);
        } else {
            throw errors::RuntimeException("DEC requires numeric value");
        }
    }

    void ArithmeticExecutor::handleAddInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: ADD_INT requires 2 values");
        }
        // Phase 6: Type guard — deopt to generic ADD if types don't match
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            handleAdd(); // Fall back to generic
            return;
        }
        int64_t r = std::get<int64_t>(context.stackManager->pop());
        int64_t l = std::get<int64_t>(context.stackManager->pop());
        context.stackManager->push(l + r);
    }

    void ArithmeticExecutor::handleSubInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SUB_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            handleSub();
            return;
        }
        int64_t r = std::get<int64_t>(context.stackManager->pop());
        int64_t l = std::get<int64_t>(context.stackManager->pop());
        context.stackManager->push(l - r);
    }

    void ArithmeticExecutor::handleMulInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: MUL_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            handleMul();
            return;
        }
        int64_t r = std::get<int64_t>(context.stackManager->pop());
        int64_t l = std::get<int64_t>(context.stackManager->pop());
        context.stackManager->push(l * r);
    }

    void ArithmeticExecutor::handleDivInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: DIV_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            handleDiv();
            return;
        }
        int64_t r = std::get<int64_t>(context.stackManager->pop());
        int64_t l = std::get<int64_t>(context.stackManager->pop());
        if (r == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
        }
        context.stackManager->push(l / r);
    }

    value::Value ArithmeticExecutor::performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op) {
        using OpCode = bytecode::OpCode;

        // Auto-unbox boxed types (Int, Float, Bool, String) to primitives
        value::Value unboxedLeft = left;
        value::Value unboxedRight = right;

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left);
            std::string typeName = obj->getTypeName();
            if (typeName == "Int" || typeName == "Float" ||
                typeName == "Bool" || typeName == "String") {
                unboxedLeft = obj->getFieldValue("value");
            }
        }
        else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(left)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(left);
            const std::string& typeName = obj->getClassName();
            if (typeName == "Int" || typeName == "Float" ||
                typeName == "Bool" || typeName == "String") {
                unboxedLeft = obj->getFieldValue("value");
            }
        }

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right);
            std::string typeName = obj->getTypeName();
            if (typeName == "Int" || typeName == "Float" ||
                typeName == "Bool" || typeName == "String") {
                unboxedRight = obj->getFieldValue("value");
            }
        }
        else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(right)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(right);
            const std::string& typeName = obj->getClassName();
            if (typeName == "Int" || typeName == "Float" ||
                typeName == "Bool" || typeName == "String") {
                unboxedRight = obj->getFieldValue("value");
            }
        }

        // Integer operations
        if (std::holds_alternative<int64_t>(unboxedLeft) && std::holds_alternative<int64_t>(unboxedRight)) {
            int64_t l = std::get<int64_t>(unboxedLeft);
            int64_t r = std::get<int64_t>(unboxedRight);
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
                    }
                    return l / r;
                case OpCode::MOD:
                    if (r == 0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero");
                    }
                    return l % r;
                default: break;
            }
        }

        // Float operations
        if ((std::holds_alternative<float>(unboxedLeft) || std::holds_alternative<int64_t>(unboxedLeft)) &&
            (std::holds_alternative<float>(unboxedRight) || std::holds_alternative<int64_t>(unboxedRight))) {
            float l = std::holds_alternative<float>(unboxedLeft) ? std::get<float>(unboxedLeft) : static_cast<float>(std::get<int64_t>(unboxedLeft));
            float r = std::holds_alternative<float>(unboxedRight) ? std::get<float>(unboxedRight) : static_cast<float>(std::get<int64_t>(unboxedRight));
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0f) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
                    }
                    return l / r;
                default: break;
            }
        }

        // String concatenation (includes objects, which should call toString())
        if (op == OpCode::ADD &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
             std::holds_alternative<value::InternedString>(left) || std::holds_alternative<value::InternedString>(right) ||
             std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left) ||
             std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right) ||
             std::holds_alternative<std::shared_ptr<value::ValueObject>>(left) ||
             std::holds_alternative<std::shared_ptr<value::ValueObject>>(right))) {

            // Convert both operands to string using valueToString
            std::string leftStr = valueToString(left);
            std::string rightStr = valueToString(right);

            // Concatenate and intern the result
            std::string result = leftStr + rightStr;
            auto& pool = value::StringPool::getInstance();
            return pool.intern(std::move(result));
        }

        throw errors::RuntimeException("Invalid binary operation");
    }

    std::string ArithmeticExecutor::valueToString(const value::Value& val) const {
        if (std::holds_alternative<int64_t>(val)) {
            return std::to_string(std::get<int64_t>(val));
        }
        if (std::holds_alternative<float>(val)) {
            // Format float to match interpreter behavior (remove trailing zeros)
            std::ostringstream oss;
            oss << std::get<float>(val);
            return oss.str();
        }
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::string>(val)) {
            return std::get<std::string>(val);
        }
        if (std::holds_alternative<value::InternedString>(val)) {
            return std::get<value::InternedString>(val).getString();
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return "null";
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj) {
                // First, try to call toString() if it exists (custom toString() takes priority)
                auto classDef = obj->getClassDefinition();
                if (classDef && classDef->hasMethod("toString")) {
                    auto toStringMethod = classDef->findInstanceMethod("toString", 0);
                    if (toStringMethod) {
                        try {
                            // WORKAROUND: obj->callMethod() is currently a stub that returns void
                            // Instead, manually construct toString() output from object fields
                            // This is a heuristic approach that handles common toString() patterns

                            std::string result;
                            bool constructed = false;

                            // Pattern 1: name:value (e.g., TestObject with name and value fields)
                            if (obj->getField("name") && obj->getField("value")) {
                                value::Value nameVal = obj->getFieldValue("name");
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(nameVal) + ":" + valueToString(valueVal);
                                constructed = true;
                            }
                            // Pattern 2: just a "value" field (for primitive wrappers)
                            else if (obj->getField("value") && classDef->getInstanceFields().size() == 1) {
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(valueVal);
                                constructed = true;
                            }

                            if (constructed) {
                                return result;
                            }
                        } catch (...) {
                            // If toString() construction fails, fall through to default handling
                        }
                    }
                }

                // Fallback: For primitive wrapper objects (String, Int, etc.), extract the "value" field
                // Only if toString() doesn't exist or failed
                if (obj->getField("value")) {
                    value::Value fieldValue = obj->getFieldValue("value");
                    // Recursively convert the field value to string
                    return valueToString(fieldValue);
                }
            }
        }
        // Handle ValueObject (value types)
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj) {
                // For primitive wrapper value objects, extract "value" field
                if (obj->hasField("value") && obj->getFieldCount() == 1) {
                    return valueToString(obj->getFieldValue("value"));
                }
                return "<" + obj->getClassName() + ">";
            }
        }
        return "<object>";
    }
}
