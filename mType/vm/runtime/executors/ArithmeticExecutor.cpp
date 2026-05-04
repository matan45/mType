#include "ArithmeticExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/CheckedArithmetic.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/FlatMultiArray.hpp"
#include "../../../value/SparseMultiArray.hpp"
#include "../../../value/ValueShim.hpp"
#include <sstream>

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
            return value::isString(v) ||
                   value::isInternedString(v);
        case value::PrimitiveTypeTag::NONE:
        default:
            return true;
        }
    }
}

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
        if (value::isInt(val)) {
            context.stackManager->push(utils::wrappingNeg64(value::asInt(val)));
        } else if (value::isFloat(val)) {
            context.stackManager->push(-value::asFloat(val));
        } else {
            throw errors::RuntimeException("NEG requires numeric value");
        }
    }

    void ArithmeticExecutor::handleInc() {
        value::Value val = context.stackManager->pop();
        if (value::isInt(val)) {
            context.stackManager->push(utils::wrappingAdd64(value::asInt(val), 1));
        } else if (value::isFloat(val)) {
            context.stackManager->push(value::asFloat(val) + 1.0);
        } else {
            throw errors::RuntimeException("INC requires numeric value");
        }
    }

    void ArithmeticExecutor::handleDec() {
        value::Value val = context.stackManager->pop();
        if (value::isInt(val)) {
            context.stackManager->push(utils::wrappingSub64(value::asInt(val), 1));
        } else if (value::isFloat(val)) {
            context.stackManager->push(value::asFloat(val) - 1.0);
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
        if (!value::isInt(left) || !value::isInt(right)) {
            handleAdd(); // Fall back to generic
            return;
        }
        int64_t r = value::asInt(context.stackManager->pop());
        int64_t l = value::asInt(context.stackManager->pop());
        context.stackManager->push(utils::wrappingAdd64(l, r));
    }

    void ArithmeticExecutor::handleAddIntConst(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const bytecode::BytecodeProgram::CachedInstructionState& state) {
        // MYT-198: fused PUSH_INT + ADD_INT. At entry, tos is the left operand
        // (the value that was on stack before the NOPed PUSH_INT would have
        // pushed its literal). The literal is in the constant pool at
        // state.fusedSlot — set by tryFuseAddIntConst.
        if (context.stackManager->size() < 1) {
            throw errors::RuntimeException("Stack underflow: ADD_INT_CONST requires 1 value");
        }
        const auto& tos = context.stackManager->peek(0);
        if (!value::isInt(tos)) {
            // Un-fuse on type miss: restore PUSH_INT + <add> at the pair, push
            // the literal so the stack matches the pre-fusion shape, bump
            // fusedDeoptCount sticky.
            //
            // Demote the pair to PUSH_INT + ADD_INT, not PUSH_INT + ADD:
            // tryFuseAddIntConst only fires after trySpecializeArithmetic has
            // already promoted ADD → ADD_INT, so the pre-fusion opcode was
            // ADD_INT. Restoring to ADD would waste the next dispatch on a
            // trySpecialize → promote cycle that re-derives ADD_INT. ADD_INT
            // already has a built-in type-guard fallback to handleAdd() for
            // genuinely non-int operands, so this is safe for heterogeneous
            // sites too. Handle *this* dispatch directly through handleAdd()
            // since tos is known non-int — the rewrite only affects the next
            // dispatch of this IP.
            int64_t literal = context.program->getConstantPool().getInteger(state.fusedSlot);
            context.stackManager->push(literal);
            const size_t ip = context.instructionPointer;
            auto& mut = context.getMutableInstructionAt(ip);
            auto& prevMut = context.getMutableInstructionAt(ip - 1);
            prevMut.opcode = bytecode::OpCode::PUSH_INT;
            // MYT-201: fused state now lives on the side table. Same entry as
            // `state` above, but we need a mutable view to bump fusedDeoptCount.
            auto& mutState = context.getOrCreateCachedState(ip);
            prevMut.operands = { static_cast<uint64_t>(mutState.fusedSlot) };
            mut.opcode = bytecode::OpCode::ADD_INT;
            if (mutState.fusedDeoptCount < 255) ++mutState.fusedDeoptCount;
            handleAdd();
            return;
        }
        int64_t literal = context.program->getConstantPool().getInteger(state.fusedSlot);
        int64_t l = value::asInt(context.stackManager->pop());
        context.stackManager->push(utils::wrappingAdd64(l, literal));
    }

    void ArithmeticExecutor::handleSubInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SUB_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isInt(left) || !value::isInt(right)) {
            handleSub();
            return;
        }
        int64_t r = value::asInt(context.stackManager->pop());
        int64_t l = value::asInt(context.stackManager->pop());
        context.stackManager->push(utils::wrappingSub64(l, r));
    }

    void ArithmeticExecutor::handleMulInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: MUL_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isInt(left) || !value::isInt(right)) {
            handleMul();
            return;
        }
        int64_t r = value::asInt(context.stackManager->pop());
        int64_t l = value::asInt(context.stackManager->pop());
        context.stackManager->push(utils::wrappingMul64(l, r));
    }

    void ArithmeticExecutor::handleDivInt() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: DIV_INT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isInt(left) || !value::isInt(right)) {
            handleDiv();
            return;
        }
        int64_t r = value::asInt(context.stackManager->pop());
        int64_t l = value::asInt(context.stackManager->pop());
        if (r == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
        }
        context.stackManager->push(l / r);
    }

    void ArithmeticExecutor::handleAddFloat() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: ADD_FLOAT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isFloat(left) || !value::isFloat(right)) {
            handleAdd();
            return;
        }
        double r = value::asFloat(context.stackManager->pop());
        double l = value::asFloat(context.stackManager->pop());
        context.stackManager->push(l + r);
    }

    void ArithmeticExecutor::handleSubFloat() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SUB_FLOAT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isFloat(left) || !value::isFloat(right)) {
            handleSub();
            return;
        }
        double r = value::asFloat(context.stackManager->pop());
        double l = value::asFloat(context.stackManager->pop());
        context.stackManager->push(l - r);
    }

    void ArithmeticExecutor::handleMulFloat() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: MUL_FLOAT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isFloat(left) || !value::isFloat(right)) {
            handleMul();
            return;
        }
        double r = value::asFloat(context.stackManager->pop());
        double l = value::asFloat(context.stackManager->pop());
        context.stackManager->push(l * r);
    }

    void ArithmeticExecutor::handleDivFloat() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: DIV_FLOAT requires 2 values");
        }
        const auto& right = context.stackManager->peek(0);
        const auto& left = context.stackManager->peek(1);
        if (!value::isFloat(left) || !value::isFloat(right)) {
            handleDiv();
            return;
        }
        double r = value::asFloat(context.stackManager->pop());
        double l = value::asFloat(context.stackManager->pop());
        if (r == 0.0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
        }
        context.stackManager->push(l / r);
    }

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
                    // INT64_MIN / -1 is UB on x86 (raises SIGFPE). Java
                    // defines this as wrapping to INT64_MIN; match that.
                    if (l == INT64_MIN && r == -1) {
                        return INT64_MIN;
                    }
                    return l / r;
                case OpCode::MOD:
                    if (r == 0) {
                        utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero");
                    }
                    // INT64_MIN % -1 is UB on x86. Java defines it as 0.
                    if (l == INT64_MIN && r == -1) {
                        return static_cast<int64_t>(0);
                    }
                    return l % r;
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
        if (op == OpCode::ADD &&
            (value::isString(left) || value::isString(right) ||
             value::isInternedString(left) || value::isInternedString(right) ||
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
        if (value::isString(val)) {
            return value::asString(val);
        }
        if (value::isInternedString(val)) {
            return value::asInternedString(val).getString();
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
