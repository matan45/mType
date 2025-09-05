#include "ValueConverter.hpp"
#include "../../errors/TypeException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include <sstream>

namespace evaluator::utils
{
    using namespace errors;
    using namespace runtimeTypes::klass;
    
    bool ValueConverter::isTruthy(const Value& value)
    {
        return std::visit([](const auto& val) -> bool {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, bool>) {
                return val;
            }
            else if constexpr (std::is_same_v<T, int>) {
                return val != 0;
            }
            else if constexpr (std::is_same_v<T, float>) {
                return val != 0.0f && !std::isnan(val);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return !val.empty();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObjectInstance>>) {
                return val != nullptr;
            }
            else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, std::monostate>) {
                return false;
            }
            else {
                return true; // Default for unknown types
            }
        }, value);
    }
    
    std::string ValueConverter::toString(const Value& value)
    {
        return std::visit([](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, std::string>) {
                return val;
            }
            else if constexpr (std::is_same_v<T, int>) {
                return std::to_string(val);
            }
            else if constexpr (std::is_same_v<T, float>) {
                // Format float to remove unnecessary trailing zeros
                std::ostringstream oss;
                oss << val;
                return oss.str();
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return val ? "true" : "false";
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObjectInstance>>) {
                if (val) {
                    return "[object " + val->getTypeName() + "]";
                }
                return "null";
            }
            else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return "null";
            }
            else if constexpr (std::is_same_v<T, std::monostate>) {
                return "undefined";
            }
            else {
                return "[unknown]";
            }
        }, value);
    }
    
    float ValueConverter::toFloat(const Value& value)
    {
        return std::visit([](const auto& val) -> float {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, float>) {
                return val;
            }
            else if constexpr (std::is_same_v<T, int>) {
                return static_cast<float>(val);
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return val ? 1.0f : 0.0f;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                try {
                    return std::stof(val);
                } catch (const std::exception&) {
                    throw TypeException("Cannot convert string '" + val + "' to float");
                }
            }
            else {
                throw TypeException("Cannot convert " + valueTypeToString(getValueType(Value{val})) + " to float");
            }
        }, value);
    }
    
    int ValueConverter::toInt(const Value& value)
    {
        return std::visit([](const auto& val) -> int {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, int>) {
                return val;
            }
            else if constexpr (std::is_same_v<T, float>) {
                return static_cast<int>(val);
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return val ? 1 : 0;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                try {
                    return std::stoi(val);
                } catch (const std::exception&) {
                    throw TypeException("Cannot convert string '" + val + "' to int");
                }
            }
            else {
                throw TypeException("Cannot convert " + valueTypeToString(getValueType(Value{val})) + " to int");
            }
        }, value);
    }
    
    ValueType ValueConverter::getValueType(const Value& value)
    {
        return std::visit([](const auto& val) -> ValueType {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, int>) {
                return ValueType::INT;
            }
            else if constexpr (std::is_same_v<T, float>) {
                return ValueType::FLOAT;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return ValueType::STRING;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return ValueType::BOOL;
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObjectInstance>>) {
                return ValueType::OBJECT;
            }
            else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return ValueType::NULL_TYPE;
            }
            else {
                return ValueType::VOID;
            }
        }, value);
    }
    
    std::string ValueConverter::valueTypeToString(ValueType type)
    {
        switch (type) {
            case ValueType::INT: return "int";
            case ValueType::FLOAT: return "float";
            case ValueType::STRING: return "string";
            case ValueType::BOOL: return "bool";
            case ValueType::OBJECT: return "object";
            case ValueType::NULL_TYPE: return "null";
            case ValueType::VOID: return "void";
            default: return "unknown";
        }
    }
    
    bool ValueConverter::compareValues(const Value& left, const Value& right)
    {
        // Handle null comparisons first
        if (std::holds_alternative<std::nullptr_t>(left) || 
            std::holds_alternative<std::nullptr_t>(right)) {
            return std::holds_alternative<std::nullptr_t>(left) && 
                   std::holds_alternative<std::nullptr_t>(right);
        }
        
        // Same type comparison
        if (left.index() == right.index()) {
            return std::visit([](const auto& l, const auto& r) -> bool {
                using T1 = std::decay_t<decltype(l)>;
                using T2 = std::decay_t<decltype(r)>;
                if constexpr (std::is_same_v<T1, T2>) {
                    if constexpr (std::is_same_v<T1, std::shared_ptr<ObjectInstance>>) {
                        // Object identity comparison
                        return l.get() == r.get();
                    } else if constexpr (std::is_same_v<T1, std::monostate>) {
                        // monostate values are always equal
                        return true;
                    } else if constexpr (std::is_same_v<T1, std::nullptr_t>) {
                        // nullptr values are always equal
                        return true;
                    } else {
                        return l == r;
                    }
                } else {
                    return false; // Different types are not equal
                }
            }, left, right);
        }
        
        // Type coercion for numeric types
        try {
            if ((std::holds_alternative<int>(left) || std::holds_alternative<float>(left)) &&
                (std::holds_alternative<int>(right) || std::holds_alternative<float>(right))) {
                float leftFloat = toFloat(left);
                float rightFloat = toFloat(right);
                return std::abs(leftFloat - rightFloat) < 1e-9f;
            }
        } catch (const TypeException&) {
            // Fall through to boolean coercion
        }
        
        // Type coercion for boolean to integer (for switch statements)
        try {
            if ((std::holds_alternative<bool>(left) && std::holds_alternative<int>(right)) ||
                (std::holds_alternative<int>(left) && std::holds_alternative<bool>(right))) {
                
                int leftInt = std::holds_alternative<bool>(left) ? 
                              (std::get<bool>(left) ? 1 : 0) : std::get<int>(left);
                int rightInt = std::holds_alternative<bool>(right) ? 
                               (std::get<bool>(right) ? 1 : 0) : std::get<int>(right);
                
                return leftInt == rightInt;
            }
        } catch (const std::bad_variant_access&) {
            // Fall through to false
        }
        
        return false;
    }
}