#include "JitHelpers.hpp"
#include <sstream>
#include <string>
#include "../../errors/RuntimeException.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/StringPool.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueShim.hpp"

namespace vm::jit
{
    // MYT-254 follow-up: minimal valueToString for the JIT generic-binop
    // string-concat fallback. Mirrors ArithmeticExecutor::valueToString for
    // INT/FLOAT/BOOL/STRING/INTERNED_STRING/NULL/VOID, plus the Int/Float/
    // Bool/String value-class wrappers via field-0 unbox (matches
    // tryReadBoxedField's "single `value` field at index 0" invariant).
    // Custom-toString OBJECT receivers fall through to the throw path;
    // the JIT can't safely re-enter the interpreter for arbitrary toString()
    // dispatch from inside this helper without risking re-entrancy bugs.
    static bool valueToStringForConcat(const value::Value& v, std::string& out)
    {
        if (value::isInt(v))
        {
            out = std::to_string(value::asInt(v));
            return true;
        }
        if (value::isFloat(v))
        {
            std::ostringstream oss;
            oss << value::asFloat(v);
            out = oss.str();
            return true;
        }
        if (value::isBool(v))
        {
            out = value::asBool(v) ? "true" : "false";
            return true;
        }
        // MYT-317: SSO-aware. Folds the three string forms into one branch.
        if (value::isAnyString(v))
        {
            out = std::string(value::asStringView(v));
            return true;
        }
        if (value::isNullType(v))
        {
            out = "null";
            return true;
        }
        if (value::isVoid(v))
        {
            out = "void";
            return true;
        }
        // Boxed primitive wrappers (Int/Float/Bool/String value classes from
        // lib/primitives): single `value` field at index 0 holding the
        // underlying primitive. Recurse on the field — handles `text + i`
        // where i is `Int` and `prefix + flag` where flag is `Bool`.
        if (value::isValueObject(v))
        {
            const auto& vo = value::asValueObject(v);
            if (vo && vo->getFieldCount() > 0)
                return valueToStringForConcat(vo->getFieldByIndex(0), out);
            return false;
        }
        if (value::isAnyObject(v))
        {
            auto* obj = value::asObjectInstanceRaw(v);
            if (obj && obj->hasFieldVector())
                return valueToStringForConcat(obj->getFieldByIndex(0), out);
            return false;
        }
        return false;
    }

    static void performGenericBinop(value::Value* result,
                                     const value::Value* left,
                                     const value::Value* right,
                                     char op)
    {
        if (value::isInt(*left) && value::isInt(*right))
        {
            int64_t l = value::asInt(*left);
            int64_t r = value::asInt(*right);
            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    if (r == 0) throw errors::RuntimeException("Modulo by zero");
                    *result = l % r; return;
            }
        }

        bool leftIsNumeric = value::isFloat(*left) || value::isInt(*left);
        bool rightIsNumeric = value::isFloat(*right) || value::isInt(*right);

        if (leftIsNumeric && rightIsNumeric)
        {
            double l = value::isFloat(*left)
                ? value::asFloat(*left)
                : static_cast<double>(value::asInt(*left));
            double r = value::isFloat(*right)
                ? value::asFloat(*right)
                : static_cast<double>(value::asInt(*right));

            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0.0) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    throw errors::RuntimeException("Modulo not supported for float");
            }
        }

        // MYT-254: string concatenation for ADD with at least one string-like
        // operand. Mirrors ArithmeticExecutor::performBinaryOp's string-concat
        // path so JIT-compiled outer loops handle `"-" + (i % 13)` and similar
        // mixed-type ADD chains the same way the interpreter does. Without
        // this, the throw below was being caught into ctx->pendingException
        // and the JIT loop continued with garbage, producing corrupted
        // String VALUE_OBJECTs whose value field held the right operand's
        // raw int (boxed_primitive_dispatch_hot.mt regression).
        // MYT-317: isAnyString covers STRING_INLINE alongside heap STRING.
        if (op == '+' &&
            (value::isAnyString(*left) || value::isAnyString(*right)))
        {
            std::string ls, rs;
            if (valueToStringForConcat(*left, ls) &&
                valueToStringForConcat(*right, rs))
            {
                *result = value::StringPool::getInstance().intern(ls + rs);
                return;
            }
        }

        throw errors::RuntimeException("Unsupported operand types for arithmetic");
    }

    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '+');
    }

    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '-');
    }

    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '*');
    }

    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '/');
    }

    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '%');
    }
}
