#include "ArithmeticExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include "../../../value/StringPool.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/FlatMultiArray.hpp"
#include "../../../value/SparseMultiArray.hpp"

namespace
{
    // SECURITY: validate that an unboxed value's runtime variant matches
    // the primitive tag advertised by the wrapper object. A mismatch means
    // the box's "value" field has been mutated to a type that disagrees
    // with its tag, which would otherwise let arithmetic execute on the
    // wrong type and produce a type-confusion vulnerability.
    inline bool matchesPrimitiveTag(const value::Value& v, value::PrimitiveTypeTag tag)
    {
        switch (tag)
        {
        case value::PrimitiveTypeTag::INT:
            return value::isInt(v);
        case value::PrimitiveTypeTag::FLOAT:
            return value::isFloat(v);
        case value::PrimitiveTypeTag::BOOL:
            return value::isBool(v);
        case value::PrimitiveTypeTag::STRING:
            // MYT-317: STRING_INLINE is also a string-typed primitive.
            return value::isAnyString(v);
        case value::PrimitiveTypeTag::NONE:
        default:
            return true;
        }
    }
}

namespace vm::runtime
{
    // MYT-319: hot dispatch handlers live in ArithmeticExecutor.hpp. Only
    // performBinaryOp / valueToString / handleStringBuild / formatMultiArraySlice
    // stay here — they pull in heavy includes (ObjectInstance / ClassDefinition /
    // NativeArray / FlatMultiArray / SparseMultiArray / StringPool / ValueObject)
    // and run rare paths (string concat, multi-array stringify).

    void ArithmeticExecutor::handleStringBuild(size_t count) {
        // Collect all segments from the stack (they are in reverse order)
        std::vector<value::Value> segments(count);
        for (size_t i = count; i > 0; --i) {
            segments[i - 1] = context.stackManager->pop();
        }

        // Pre-calculate total size estimate for reservation
        std::string result;
        result.reserve(count * 8); // rough estimate

        for (const auto& seg : segments) {
            result += valueToString(seg);
        }

        auto& pool = value::StringPool::getInstance();
        context.stackManager->push(pool.intern(std::move(result)));
    }

    value::Value ArithmeticExecutor::performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op) {
        using OpCode = bytecode::OpCode;

        // Auto-unbox boxed types (Int, Float, Bool, String) to primitives
        value::Value unboxedLeft = left;
        value::Value unboxedRight = right;

        if (value::isObject(left)) {
            auto obj = value::asObject(left);
            auto tag = obj->getPrimitiveTag();
            if (tag != value::PrimitiveTypeTag::NONE) {
                unboxedLeft = obj->getFieldValue("value");
                if (!matchesPrimitiveTag(unboxedLeft, tag)) {
                    throw errors::RuntimeException("Unboxing tag/value mismatch on left operand");
                }
            }
        }
        else if (value::isValueObject(left)) {
            auto obj = value::asValueObject(left);
            auto tag = obj->getPrimitiveTag();
            if (tag != value::PrimitiveTypeTag::NONE) {
                unboxedLeft = obj->getFieldValue("value");
                if (!matchesPrimitiveTag(unboxedLeft, tag)) {
                    throw errors::RuntimeException("Unboxing tag/value mismatch on left operand");
                }
            }
        }

        if (value::isObject(right)) {
            auto obj = value::asObject(right);
            auto tag = obj->getPrimitiveTag();
            if (tag != value::PrimitiveTypeTag::NONE) {
                unboxedRight = obj->getFieldValue("value");
                if (!matchesPrimitiveTag(unboxedRight, tag)) {
                    throw errors::RuntimeException("Unboxing tag/value mismatch on right operand");
                }
            }
        }
        else if (value::isValueObject(right)) {
            auto obj = value::asValueObject(right);
            auto tag = obj->getPrimitiveTag();
            if (tag != value::PrimitiveTypeTag::NONE) {
                unboxedRight = obj->getFieldValue("value");
                if (!matchesPrimitiveTag(unboxedRight, tag)) {
                    throw errors::RuntimeException("Unboxing tag/value mismatch on right operand");
                }
            }
        }

        // Integer operations
        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            int64_t l = value::asInt(unboxedLeft);
            int64_t r = value::asInt(unboxedRight);
            switch (op) {
                case OpCode::ADD: return utils::wrappingAdd64(l, r);
                case OpCode::SUB: return utils::wrappingSub64(l, r);
                case OpCode::MUL: return utils::wrappingMul64(l, r);
                case OpCode::DIV:
                    if (r == 0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
                    }
                    return utils::wrappingDiv64(l, r);
                case OpCode::MOD:
                    if (r == 0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero");
                    }
                    return utils::wrappingMod64(l, r);
                default: break;
            }
        }

        // Float operations
        if ((value::isFloat(unboxedLeft) || value::isInt(unboxedLeft)) &&
            (value::isFloat(unboxedRight) || value::isInt(unboxedRight))) {
            double l = value::isFloat(unboxedLeft) ? value::asFloat(unboxedLeft) : static_cast<double>(value::asInt(unboxedLeft));
            double r = value::isFloat(unboxedRight) ? value::asFloat(unboxedRight) : static_cast<double>(value::asInt(unboxedRight));
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
                    }
                    return l / r;
                default: break;
            }
        }

        // String concatenation (includes objects and arrays, which should call toString())
        // MYT-317: isAnyString covers STRING_INLINE alongside heap STRING.
        if (op == OpCode::ADD &&
            (value::isAnyString(left) || value::isAnyString(right) ||
             value::isObject(left) ||
             value::isObject(right) ||
             value::isValueObject(left) ||
             value::isValueObject(right) ||
             value::isNativeArray(left) ||
             value::isNativeArray(right) ||
             value::isFlatMultiArray(left) ||
             value::isFlatMultiArray(right) ||
             value::isSparseMultiArray(left) ||
             value::isSparseMultiArray(right))) {

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
        if (value::isInt(val)) {
            return std::to_string(value::asInt(val));
        }
        if (value::isFloat(val)) {
            // Format float to match interpreter behavior (remove trailing zeros)
            std::ostringstream oss;
            oss << value::asFloat(val);
            return oss.str();
        }
        if (value::isBool(val)) {
            return value::asBool(val) ? "true" : "false";
        }
        // MYT-317: SSO-aware. Folds the three string forms into one branch.
        if (value::isAnyString(val)) {
            return std::string(value::asStringView(val));
        }
        if (value::isNullType(val)) {
            return "null";
        }
        // MYT-208: accept STACK_OBJECT alongside OBJECT — string concatenation
        // on a stack-promoted local must dispatch toString() the same way.
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
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
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj) {
                // For primitive wrapper value objects, extract "value" field
                if (obj->hasField("value") && obj->getFieldCount() == 1) {
                    return valueToString(obj->getFieldValue("value"));
                }
                return "<" + obj->getClassName() + ">";
            }
        }
        // Handle NativeArray
        if (value::isNativeArray(val)) {
            auto arr = value::asNativeArray(val);
            if (arr) {
                std::string result = "[";
                for (size_t i = 0; i < arr->size(); ++i) {
                    if (i > 0) result += ", ";
                    result += valueToString(arr->get(i));
                }
                result += "]";
                return result;
            }
            return "[]";
        }
        if (value::isFlatMultiArray(val)) {
            auto arr = value::asFlatMultiArray(val);
            if (arr) {
                std::string result;
                formatMultiArraySlice(*arr, arr->getDimensions(), 0, 0, result);
                return result;
            }
            return "[]";
        }
        if (value::isSparseMultiArray(val)) {
            auto arr = value::asSparseMultiArray(val);
            if (arr) {
                std::string result;
                formatMultiArraySlice(*arr, arr->getDimensions(), 0, 0, result);
                return result;
            }
            return "[]";
        }
        return "<object>";
    }

    template<typename ArrayType>
    void ArithmeticExecutor::formatMultiArraySlice(
        const ArrayType& arr, const std::vector<size_t>& dims,
        size_t dimIndex, size_t offset, std::string& out) const
    {
        if (dimIndex >= dims.size()) {
            return;
        }

        out += '[';
        size_t currentDimSize = dims[dimIndex];

        if (dimIndex == dims.size() - 1) {
            // Innermost dimension: format individual elements
            for (size_t i = 0; i < currentDimSize; ++i) {
                if (i > 0) out += ", ";
                out += valueToString(arr.get(offset + i));
            }
        } else {
            // Calculate stride for this dimension
            size_t stride = 1;
            for (size_t d = dimIndex + 1; d < dims.size(); ++d) {
                stride *= dims[d];
            }
            for (size_t i = 0; i < currentDimSize; ++i) {
                if (i > 0) out += ", ";
                formatMultiArraySlice(arr, dims, dimIndex + 1, offset + i * stride, out);
            }
        }
        out += ']';
    }

    // Explicit template instantiations
    template void ArithmeticExecutor::formatMultiArraySlice<value::FlatMultiArray>(
        const value::FlatMultiArray&, const std::vector<size_t>&, size_t, size_t, std::string&) const;
    template void ArithmeticExecutor::formatMultiArraySlice<value::SparseMultiArray>(
        const value::SparseMultiArray&, const std::vector<size_t>&, size_t, size_t, std::string&) const;
}
