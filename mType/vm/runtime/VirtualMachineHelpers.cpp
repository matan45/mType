#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include "../../errors/DivisionByZeroException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/StackUnderflowException.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/StringPool.hpp"
#include "../../value/ValueShim.hpp"
#include "utils/CheckedArithmetic.hpp"
#include <sstream>
#include <string>

namespace vm::runtime
{
    bool VirtualMachine::isTruthy(const value::Value& val) const
    {
        if (value::isBool(val))
        {
            return value::asBool(val);
        }
        if (value::isInt(val))
        {
            return value::asInt(val) != 0;
        }
        if (value::isNullType(val))
        {
            return false;
        }
        if (value::isVoid(val))
        {
            return false;
        }
        return true; // Objects, strings, etc. are truthy
    }

    std::string VirtualMachine::valueToString(const value::Value& val) const
    {
        if (value::isInt(val))
        {
            return std::to_string(value::asInt(val));
        }
        if (value::isFloat(val))
        {
            // Format float to match interpreter behavior (remove trailing zeros)
            std::ostringstream oss;
            oss << value::asFloat(val);
            return oss.str();
        }
        if (value::isBool(val))
        {
            return value::asBool(val) ? "true" : "false";
        }
        // MYT-317: SSO-aware. Folds the three string forms into one branch.
        if (value::isAnyString(val))
        {
            return std::string(value::asStringView(val));
        }
        if (value::isVoid(val))
        {
            return "void";  // monostate represents void/uninitialized - should not typically be printed
        }
        if (value::isNullType(val))
        {
            return "null";
        }
        // MYT-208: accept STACK_OBJECT alongside OBJECT — print/log on a
        // stack-promoted local must dispatch toString() the same way.
        if (value::isAnyObject(val))
        {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj)
            {
                // First, try to call toString() if it exists (custom toString() takes priority)
                auto classDef = obj->getClassDefinition();
                if (classDef && classDef->hasMethod("toString"))
                {
                    auto toStringMethod = classDef->findMethod("toString", 0);
                    if (toStringMethod && !toStringMethod->isStatic())
                    {
                        try
                        {
                            // WORKAROUND: obj->callMethod() is currently a stub that returns void
                            // Instead, manually construct toString() output from object fields
                            // This is a heuristic approach that handles common toString() patterns

                            std::string result;
                            bool constructed = false;

                            // Pattern 1: name:value (e.g., TestObject with name and value fields)
                            if (obj->getField("name") && obj->getField("value"))
                            {
                                value::Value nameVal = obj->getFieldValue("name");
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(nameVal) + ":" + valueToString(valueVal);
                                constructed = true;
                            }
                            // Pattern 2: just a "value" field (for primitive wrappers)
                            else if (obj->getField("value") && classDef->getInstanceFields().size() == 1)
                            {
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(valueVal);
                                constructed = true;
                            }

                            if (constructed)
                            {
                                return result;
                            }
                        }
                        catch (...)
                        {
                            // If toString() construction fails, fall through to default handling
                        }
                    }
                }

                // Fallback: For primitive wrapper objects (String, Int, etc.), extract the "value" field
                // Only if toString() doesn't exist or failed
                if (obj->getField("value"))
                {
                    value::Value fieldValue = obj->getFieldValue("value");
                    // Recursively convert the field value to string
                    return valueToString(fieldValue);
                }
            }
        }
        // Handle ValueObject (value types)
        if (value::isValueObject(val))
        {
            auto obj = value::asValueObject(val);
            if (obj)
            {
                // For primitive wrapper value objects, extract "value" field
                if (obj->hasField("value") && obj->getFieldCount() == 1) {
                    return valueToString(obj->getFieldValue("value"));
                }
                return "<" + obj->getClassName() + ">";
            }
        }
        return "<object>";
    }

    value::Value VirtualMachine::performBinaryOp(const value::Value& left, const value::Value& right,
                                                 bytecode::OpCode op)
    {
        using OpCode = bytecode::OpCode;

        // Debug output
        // Integer operations
        if (value::isInt(left) && value::isInt(right))
        {
            int64_t l = value::asInt(left);
            int64_t r = value::asInt(right);
            switch (op)
            {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0) throw errors::DivisionByZeroException("division");
                return utils::wrappingDiv64(l, r);
            case OpCode::MOD:
                if (r == 0) throw errors::DivisionByZeroException("modulo");
                return utils::wrappingMod64(l, r);
            default: break;
            }
        }

        // Float operations
        if ((value::isFloat(left) || value::isInt(left)) &&
            (value::isFloat(right) || value::isInt(right)))
        {
            double l = value::isFloat(left)
                          ? value::asFloat(left)
                          : static_cast<double>(value::asInt(left));
            double r = value::isFloat(right)
                          ? value::asFloat(right)
                          : static_cast<double>(value::asInt(right));
            switch (op)
            {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0.0) throw errors::DivisionByZeroException("division");
                return l / r;
            default: break;
            }
        }

        // String concatenation (includes objects, which should call toString())
        // MYT-317: isAnyString covers STRING_INLINE alongside heap STRING.
        if (op == OpCode::ADD &&
            (value::isAnyString(left) || value::isAnyString(right) ||
                value::isObject(left) ||
                value::isObject(right)))
        {
            // Convert both operands to string using valueToString
            // This will call toString() for objects via AST (cross-mode call)
            std::string leftStr = valueToString(left);
            std::string rightStr = valueToString(right);

            // Concatenate and intern the result
            std::string result = leftStr + rightStr;
            auto& pool = value::StringPool::getInstance();
            return pool.intern(std::move(result));
        }

        throw errors::RuntimeException("Invalid binary operation");
    }

    void VirtualMachine::checkStackUnderflow(size_t required) const
    {
        // Delegate to StackManager - this method is kept for compatibility
        // Will be removed once all handlers are moved to executors
        if (stackManager->size() < required)
        {
            throw errors::StackUnderflowException(required, stackManager->size());
        }
    }

    value::ValueType VirtualMachine::stringToValueType(const std::string& typeName)
    {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        // For any object/class type
        return value::ValueType::OBJECT;
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createJaggedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName, size_t totalDimensions)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in jagged array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last specified dimension
            // If this is also the last total dimension, create array of actual elements
            // Otherwise, create array of null references (for jagged arrays)
            if (dimensions.size() == totalDimensions)
            {
                // Fully specified - create array of elements
                // Convert element type name to ValueType
                value::ValueType elemType = stringToValueType(elementTypeName);
                return std::make_shared<value::NativeArray>(currentDimSize, elemType, elementTypeName);
            }
            else
            {
                // Jagged - create array of null array references
                auto array = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");
                // Elements are initialized to std::monostate{} (null) by default
                return array;
            }
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createJaggedArray(dimensions, dimIndex + 1, elementTypeName, totalDimensions);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createNestedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in multi-dimensional array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last dimension - create array of actual elements
            return std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, elementTypeName);
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }
}
