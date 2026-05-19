#include "JitHelpers.hpp"
#include "../../value/HashUtils.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/PrimitiveTypeTag.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include <cstdint>
#include <memory>

namespace vm::jit
{
    static value::PrimitiveTypeTag getPrimitiveProtocolTag(const value::Value& v) noexcept
    {
        if (value::isValueObject(v))
        {
            const auto& valueObj = value::asValueObject(v);
            return valueObj ? valueObj->getPrimitiveTag() : value::PrimitiveTypeTag::NONE;
        }
        if (value::isAnyObject(v))
        {
            auto* raw = value::asObjectInstanceRaw(v);
            if (!raw || !raw->getClassDefinition()) return value::PrimitiveTypeTag::NONE;
            return value::classNameToPrimitiveTag(raw->getClassDefinition()->getName());
        }
        return value::PrimitiveTypeTag::NONE;
    }

    static const value::Value* getPrimitiveProtocolField0(const value::Value& v) noexcept
    {
        if (value::isValueObject(v))
        {
            const auto& valueObj = value::asValueObject(v);
            if (!valueObj || valueObj->getFieldCount() == 0) return nullptr;
            return &valueObj->getFieldByIndex(0);
        }
        if (value::isAnyObject(v))
        {
            auto* raw = value::asObjectInstanceRaw(v);
            if (!raw || !raw->hasFieldVector()) return nullptr;
            return &raw->getFieldByIndex(0);
        }
        return nullptr;
    }

    static bool computePrimitiveProtocolHash(value::Value& out, const value::Value& receiver)
    {
        const value::PrimitiveTypeTag tag = getPrimitiveProtocolTag(receiver);
        if (tag == value::PrimitiveTypeTag::NONE) return false;
        const value::Value* field = getPrimitiveProtocolField0(receiver);
        if (!field) return false;

        switch (tag)
        {
            case value::PrimitiveTypeTag::INT:
                if (!value::isInt(*field)) return false;
                out = ::value::hashutils::intHash(value::asInt(*field));
                return true;
            case value::PrimitiveTypeTag::FLOAT:
                if (!value::isFloat(*field)) return false;
                out = ::value::hashutils::floatHash(value::asFloat(*field));
                return true;
            case value::PrimitiveTypeTag::BOOL:
                if (!value::isBool(*field)) return false;
                out = ::value::hashutils::boolHash(value::asBool(*field));
                return true;
            case value::PrimitiveTypeTag::STRING:
                // SSO-aware: equal-content strings hash identically regardless of
                // inline/heap representation.
                if (value::isAnyString(*field))
                {
                    out = ::value::hashutils::stringHash(value::asStringView(*field));
                    return true;
                }
                return false;
            case value::PrimitiveTypeTag::NONE:
                return false;
        }
        return false;
    }

    static bool computePrimitiveProtocolEquals(value::Value& out,
                                               const value::Value& receiver,
                                               const value::Value& arg)
    {
        const value::PrimitiveTypeTag tag = getPrimitiveProtocolTag(receiver);
        if (tag == value::PrimitiveTypeTag::NONE || getPrimitiveProtocolTag(arg) != tag)
            return false;

        const value::Value* left = getPrimitiveProtocolField0(receiver);
        const value::Value* right = getPrimitiveProtocolField0(arg);
        if (!left || !right) return false;

        switch (tag)
        {
            case value::PrimitiveTypeTag::INT:
                if (!value::isInt(*left) || !value::isInt(*right)) return false;
                out = value::asInt(*left) == value::asInt(*right);
                return true;
            case value::PrimitiveTypeTag::FLOAT:
                if (!value::isFloat(*left) || !value::isFloat(*right)) return false;
                out = value::asFloat(*left) == value::asFloat(*right);
                return true;
            case value::PrimitiveTypeTag::BOOL:
                if (!value::isBool(*left) || !value::isBool(*right)) return false;
                out = value::asBool(*left) == value::asBool(*right);
                return true;
            case value::PrimitiveTypeTag::STRING:
                if (!(value::isString(*left) || value::isInternedString(*left)) ||
                    !(value::isString(*right) || value::isInternedString(*right)))
                    return false;
                out = *left == *right;
                return true;
            case value::PrimitiveTypeTag::NONE:
                return false;
        }
        return false;
    }

    bool jit_try_primitive_protocol_hash(value::Value* out,
                                         const value::Value* receiver)
    {
        if (!out || !receiver) return false;
        return computePrimitiveProtocolHash(*out, *receiver);
    }

    bool jit_try_primitive_protocol_equals(value::Value* out,
                                           const value::Value* receiver,
                                           const value::Value* arg)
    {
        if (!out || !receiver || !arg) return false;
        return computePrimitiveProtocolEquals(*out, *receiver, *arg);
    }

    // Structural-equality JIT helpers; mirror ObjectExecutor::handleStruct*Int.
    int64_t jit_struct_hash_int(const value::Value* receiver,
                                uint64_t fieldCount,
                                const uint64_t* slots)
    {
        if (!receiver || (fieldCount > 0 && !slots)) return 1;

        int64_t h = 1;

        if (auto* raw = value::asObjectInstanceRaw(*receiver))
        {
            if (!raw->hasFieldVector()) return h;
            for (uint64_t i = 0; i < fieldCount; ++i)
            {
                const value::Value& fv = raw->getFieldByIndex(static_cast<size_t>(slots[i]));
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
            return h;
        }

        if (value::isValueObject(*receiver))
        {
            auto vobj = value::asValueObject(*receiver);
            for (uint64_t i = 0; i < fieldCount; ++i)
            {
                const value::Value& fv = vobj->getFieldByIndex(static_cast<size_t>(slots[i]));
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }
        return h;
    }

    // Returns 1 for equal, 0 for not equal. Passed back through int64_t to match
    // asmjit's Gp register width and the helper's FuncSignature.
    int64_t jit_struct_eq_int(const value::Value* thisVal,
                              const value::Value* otherVal,
                              const char* className,
                              uint64_t fieldCount,
                              const uint64_t* slots)
    {
        if (!thisVal || !otherVal || !className) return 0;
        if (value::isNullType(*otherVal)) return 0;

        auto* thisRaw = value::asObjectInstanceRaw(*thisVal);
        auto* otherRaw = value::asObjectInstanceRaw(*otherVal);
        if (!thisRaw || !otherRaw) return 0;
        if (!thisRaw->hasFieldVector() || !otherRaw->hasFieldVector()) return 0;

        const std::string& otherClassName = otherRaw->getClassDefinition()->getName();
        const std::string ownerClass(className);
        if (otherClassName != ownerClass)
        {
            // Walk the parent chain only when names differ. Holds the locked
            // shared_ptr across iterations to keep the def alive.
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> defHolder =
                otherRaw->getClassDefinition();
            bool match = false;
            while (defHolder)
            {
                if (defHolder->getName() == ownerClass) { match = true; break; }
                defHolder = defHolder->getParentClass();
            }
            if (!match) return 0;
        }

        for (uint64_t i = 0; i < fieldCount; ++i)
        {
            const value::Value& a = thisRaw->getFieldByIndex(static_cast<size_t>(slots[i]));
            const value::Value& b = otherRaw->getFieldByIndex(static_cast<size_t>(slots[i]));
            const int64_t ai = value::isInt(a) ? value::asInt(a) : 0;
            const int64_t bi = value::isInt(b) ? value::asInt(b) : 0;
            if (ai != bi) return 0;
        }
        return 1;
    }
}
