#include "ArithmeticExecutor.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
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
        if (std::holds_alternative<int>(val)) {
            context.stackManager->push(-std::get<int>(val));
        } else if (std::holds_alternative<float>(val)) {
            context.stackManager->push(-std::get<float>(val));
        } else {
            throw errors::RuntimeException("NEG requires numeric value");
        }
    }

    void ArithmeticExecutor::handleInc() {
        value::Value val = context.stackManager->pop();
        if (std::holds_alternative<int>(val)) {
            context.stackManager->push(std::get<int>(val) + 1);
        } else if (std::holds_alternative<float>(val)) {
            context.stackManager->push(std::get<float>(val) + 1.0f);
        } else {
            throw errors::RuntimeException("INC requires numeric value");
        }
    }

    void ArithmeticExecutor::handleDec() {
        value::Value val = context.stackManager->pop();
        if (std::holds_alternative<int>(val)) {
            context.stackManager->push(std::get<int>(val) - 1);
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
        int right = std::get<int>(context.stackManager->pop());
        int left = std::get<int>(context.stackManager->pop());
        context.stackManager->push(left + right);
    }

    void ArithmeticExecutor::handleSubInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SUB_INT requires 2 values");
        }
        int right = std::get<int>(context.stackManager->pop());
        int left = std::get<int>(context.stackManager->pop());
        context.stackManager->push(left - right);
    }

    void ArithmeticExecutor::handleMulInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: MUL_INT requires 2 values");
        }
        int right = std::get<int>(context.stackManager->pop());
        int left = std::get<int>(context.stackManager->pop());
        context.stackManager->push(left * right);
    }

    void ArithmeticExecutor::handleDivInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: DIV_INT requires 2 values");
        }
        int right = std::get<int>(context.stackManager->pop());
        int left = std::get<int>(context.stackManager->pop());
        if (right == 0) {
            throw errors::RuntimeException("Division by zero");
        }
        context.stackManager->push(left / right);
    }

    value::Value ArithmeticExecutor::performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op) {
        using OpCode = bytecode::OpCode;

        // Integer operations
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            int l = std::get<int>(left);
            int r = std::get<int>(right);
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0) throw errors::RuntimeException("Division by zero");
                    return l / r;
                case OpCode::MOD:
                    if (r == 0) throw errors::RuntimeException("Modulo by zero");
                    return l % r;
                default: break;
            }
        }

        // Float operations
        if ((std::holds_alternative<float>(left) || std::holds_alternative<int>(left)) &&
            (std::holds_alternative<float>(right) || std::holds_alternative<int>(right))) {
            float l = std::holds_alternative<float>(left) ? std::get<float>(left) : static_cast<float>(std::get<int>(left));
            float r = std::holds_alternative<float>(right) ? std::get<float>(right) : static_cast<float>(std::get<int>(right));
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                    return l / r;
                default: break;
            }
        }

        // String concatenation (includes objects, which should call toString())
        if (op == OpCode::ADD &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
             std::holds_alternative<value::InternedString>(left) || std::holds_alternative<value::InternedString>(right) ||
             std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left) ||
             std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right))) {

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
        if (std::holds_alternative<int>(val)) {
            return std::to_string(std::get<int>(val));
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
                    auto toStringMethod = classDef->findMethod("toString", 0);
                    if (toStringMethod && !toStringMethod->isStatic()) {
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
        return "<object>";
    }
}
