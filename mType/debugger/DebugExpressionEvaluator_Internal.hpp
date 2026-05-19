#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "DebugExpressionEvaluator.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/NativeArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"
#include "../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../vm/runtime/utils/CheckedArithmetic.hpp"

namespace debugger::detail
{
    enum class TokenType
    {
        End,
        Identifier,
        Integer,
        Float,
        String,
        LParen,
        RParen,
        LBracket,
        RBracket,
        Dot,
        Scope,
        Plus,
        Minus,
        Star,
        Slash,
        Percent,
        Bang,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        EqualEqual,
        BangEqual,
        AndAnd,
        OrOr,
        Assign
    };

    struct Token
    {
        TokenType type;
        std::string text;
    };

    inline bool isNullish(const value::Value& val)
    {
        return value::isVoid(val) || value::isNullType(val);
    }

    inline bool isNumeric(const value::Value& val)
    {
        return value::isInt(val) || value::isFloat(val);
    }

    inline double asDouble(const value::Value& val)
    {
        if (value::isFloat(val)) return value::asFloat(val);
        if (value::isInt(val)) return static_cast<double>(value::asInt(val));
        throw std::runtime_error("Expected numeric value");
    }

    inline std::string asPlainString(const value::Value& val)
    {
        // MYT-317: SSO-aware. Folds all three string forms into one branch.
        if (value::isAnyString(val)) return std::string(value::asStringView(val));
        throw std::runtime_error("Expected string value");
    }

    inline bool equals(const value::Value& left, const value::Value& right)
    {
        if (isNullish(left) || isNullish(right))
        {
            return isNullish(left) && isNullish(right);
        }
        if (value::isInt(left) && value::isInt(right))
        {
            return value::asInt(left) == value::asInt(right);
        }
        if (isNumeric(left) && isNumeric(right))
        {
            return asDouble(left) == asDouble(right);
        }
        // MYT-317: isAnyString covers STRING_INLINE.
        if (value::isAnyString(left) && value::isAnyString(right))
        {
            return asPlainString(left) == asPlainString(right);
        }
        return left == right;
    }

    inline value::Value compare(const value::Value& left,
                                const value::Value& right,
                                TokenType op)
    {
        if (!isNumeric(left) || !isNumeric(right))
        {
            throw std::runtime_error("Comparison requires numeric operands");
        }

        if (value::isInt(left) && value::isInt(right))
        {
            int64_t l = value::asInt(left);
            int64_t r = value::asInt(right);
            switch (op)
            {
            case TokenType::Less: return l < r;
            case TokenType::LessEqual: return l <= r;
            case TokenType::Greater: return l > r;
            case TokenType::GreaterEqual: return l >= r;
            default: break;
            }
            throw std::runtime_error("Invalid comparison operator");
        }

        double l = asDouble(left);
        double r = asDouble(right);
        switch (op)
        {
        case TokenType::Less: return l < r;
        case TokenType::LessEqual: return l <= r;
        case TokenType::Greater: return l > r;
        case TokenType::GreaterEqual: return l >= r;
        default: break;
        }
        throw std::runtime_error("Invalid comparison operator");
    }

    inline value::Value add(const value::Value& left, const value::Value& right)
    {
        if (value::isInt(left) && value::isInt(right))
        {
            return vm::runtime::utils::wrappingAdd64(value::asInt(left), value::asInt(right));
        }
        if (isNumeric(left) && isNumeric(right))
        {
            return asDouble(left) + asDouble(right);
        }
        // MYT-317: isAnyString covers STRING_INLINE.
        if (value::isAnyString(left) && value::isAnyString(right))
        {
            auto& pool = value::StringPool::getInstance();
            return pool.intern(asPlainString(left) + asPlainString(right));
        }
        throw std::runtime_error("'+' requires numeric operands or two strings");
    }

    inline value::Value subtract(const value::Value& left, const value::Value& right)
    {
        if (value::isInt(left) && value::isInt(right))
        {
            return vm::runtime::utils::wrappingSub64(value::asInt(left), value::asInt(right));
        }
        if (isNumeric(left) && isNumeric(right))
        {
            return asDouble(left) - asDouble(right);
        }
        throw std::runtime_error("'-' requires numeric operands");
    }

    inline value::Value multiplyDivideModulo(const value::Value& left,
                                             const value::Value& right,
                                             TokenType op)
    {
        if (!isNumeric(left) || !isNumeric(right))
        {
            throw std::runtime_error("Arithmetic requires numeric operands");
        }

        if (op == TokenType::Percent)
        {
            if (!value::isInt(left) || !value::isInt(right))
            {
                throw std::runtime_error("'%' requires integer operands");
            }
            int64_t r = value::asInt(right);
            if (r == 0) throw std::runtime_error("Modulo by zero");
            int64_t l = value::asInt(left);
            if (l == INT64_MIN && r == -1) return static_cast<int64_t>(0);
            return l % r;
        }

        if (value::isInt(left) && value::isInt(right))
        {
            int64_t l = value::asInt(left);
            int64_t r = value::asInt(right);
            if (op == TokenType::Star)
            {
                return vm::runtime::utils::wrappingMul64(l, r);
            }
            if (r == 0) throw std::runtime_error("Division by zero");
            if (l == INT64_MIN && r == -1)
            {
                throw std::runtime_error("Integer overflow in division");
            }
            return l / r;
        }

        double l = asDouble(left);
        double r = asDouble(right);
        if (op == TokenType::Star)
        {
            return l * r;
        }
        if (r == 0.0)
        {
            throw std::runtime_error("Division by zero");
        }
        return l / r;
    }

    inline value::Value readField(const value::Value& receiver, const std::string& fieldName)
    {
        if (!value::isAnyObject(receiver))
        {
            throw std::runtime_error("Field access requires an object receiver");
        }

        auto* obj = value::asObjectInstanceRaw(receiver);
        if (!obj)
        {
            throw std::runtime_error("Field access on null object");
        }

        value::Value field = obj->getFieldValue(fieldName);
        if (value::isVoid(field))
        {
            throw std::runtime_error("Field not found: " + fieldName);
        }
        return field;
    }

    inline value::Value readIndex(const value::Value& receiver, const value::Value& indexValue)
    {
        if (!value::isInt(indexValue))
        {
            throw std::runtime_error("Array index must be an integer");
        }

        int64_t signedIndex = value::asInt(indexValue);
        if (signedIndex < 0)
        {
            throw std::runtime_error("Array index cannot be negative");
        }
        size_t index = static_cast<size_t>(signedIndex);

        if (value::isNativeArray(receiver))
        {
            auto arr = value::asNativeArray(receiver);
            if (!arr || index >= arr->size())
            {
                throw std::runtime_error("Array index out of bounds");
            }
            return arr->get(index);
        }
        if (value::isFlatMultiArray(receiver))
        {
            auto arr = value::asFlatMultiArray(receiver);
            return arr->get(index);
        }
        if (value::isSparseMultiArray(receiver))
        {
            auto arr = value::asSparseMultiArray(receiver);
            return arr->get(index);
        }
        if (value::isFlatMultiObjectArray(receiver))
        {
            auto arr = value::asFlatMultiObjectArray(receiver);
            return arr->getLinear(index);
        }

        throw std::runtime_error("Index access requires an array receiver");
    }

    // Evaluate a parsed token stream against the resolver. Defined in
    // DebugExpressionEvaluator_Scope.cpp where the Parser class lives.
    value::Value parseTokens(std::vector<Token> tokens,
                             const DebugExpressionEvaluator::Resolver& resolver);
}
