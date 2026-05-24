/**
 * VMVariableInspector — MYT-365 boxed-primitive and value-class formatting.
 *
 * Split off from VMVariableInspector_Format.cpp to keep that TU under the
 * 500-line cap. Hosts the helpers that translate VALUE_OBJECT and OBJECT
 * forms of the lib/primitives wrappers (Int/Float/Bool/String) into their
 * inner primitive's string form, plus the fallback formatting for any
 * non-primitive value class. The "field index 0 holds the wrapped primitive"
 * invariant is documented and asserted in PrimitiveTypeTag.hpp and
 * vm/runtime/executors/PrimitiveMethodExecutor.cpp.
 */

#include "VMVariableInspector.hpp"
#include <iostream>
#include "../value/ObjectInstance.hpp"
#include "../value/ValueObject.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"
#include "../environment/registry/ClassDefinition.hpp"

namespace debugger
{
    namespace
    {
        std::optional<value::Value> tryUnwrapPrimitiveWrapper(const value::Value& val)
        {
            if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (!obj || obj->getPrimitiveTag() == value::PrimitiveTypeTag::NONE)
                {
                    return std::nullopt;
                }
                obj->ensureFieldVector();
                return obj->getFieldByIndex(0);
            }
            if (value::isValueObject(val))
            {
                const auto& vo = value::asValueObject(val);
                if (!vo || vo->getPrimitiveTag() == value::PrimitiveTypeTag::NONE)
                {
                    return std::nullopt;
                }
                return vo->getFieldByIndex(0);
            }
            return std::nullopt;
        }
    } // namespace

    bool VMVariableInspector::isBoxedPrimitiveWrapper(const value::Value& val)
    {
        if (value::isAnyObject(val))
        {
            auto* obj = value::asObjectInstanceRaw(val);
            return obj && obj->getPrimitiveTag() != value::PrimitiveTypeTag::NONE;
        }
        if (value::isValueObject(val))
        {
            const auto& vo = value::asValueObject(val);
            return vo && vo->getPrimitiveTag() != value::PrimitiveTypeTag::NONE;
        }
        return false;
    }

    std::optional<std::string> VMVariableInspector::tryFormatBoxedPrimitive(const value::Value& val)
    {
        auto inner = tryUnwrapPrimitiveWrapper(val);
        if (!inner) return std::nullopt;
        if (value::isInt(*inner)) return std::to_string(value::asInt(*inner));
        if (value::isFloat(*inner)) return std::to_string(value::asFloat(*inner));
        if (value::isBool(*inner)) return std::string{value::asBool(*inner) ? "true" : "false"};
        if (value::isAnyString(*inner))
        {
            try
            {
                return "\"" + std::string(value::asStringView(*inner)) + "\"";
            }
            catch (const std::exception& e)
            {
                return "\"<error: " + std::string(e.what()) + ">\"";
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> VMVariableInspector::tryGetBoxedPrimitiveTypeName(const value::Value& val)
    {
        if (value::isAnyObject(val))
        {
            auto* obj = value::asObjectInstanceRaw(val);
            if (!obj || obj->getPrimitiveTag() == value::PrimitiveTypeTag::NONE)
            {
                return std::nullopt;
            }
            try { return obj->getTypeName(); }
            catch (const std::exception&) { return std::nullopt; }
        }
        if (value::isValueObject(val))
        {
            const auto& vo = value::asValueObject(val);
            if (!vo || vo->getPrimitiveTag() == value::PrimitiveTypeTag::NONE)
            {
                return std::nullopt;
            }
            return vo->getClassName();
        }
        return std::nullopt;
    }

    std::optional<std::string> VMVariableInspector::tryFormatValueObjectComposite(const value::Value& val)
    {
        if (!value::isValueObject(val)) return std::nullopt;
        const auto& vo = value::asValueObject(val);
        if (!vo) return std::string{"Object[null]"};
        try
        {
            return vo->getClassName() + " instance";
        }
        catch (const std::exception& e)
        {
            return "Object[error: " + std::string(e.what()) + "]";
        }
    }

    std::optional<std::string> VMVariableInspector::tryGetValueObjectTypeName(const value::Value& val)
    {
        if (!value::isValueObject(val)) return std::nullopt;
        const auto& vo = value::asValueObject(val);
        if (!vo) return std::string{"Object"};
        try
        {
            return vo->getClassName();
        }
        catch (const std::exception&)
        {
            return std::string{"Object[error]"};
        }
    }

    void VMVariableInspector::collectValueObjectChildren(const value::Value& val,
                                                         std::vector<DebugVariable>& children)
    {
        const auto& vo = value::asValueObject(val);
        if (!vo) return;
        try
        {
            auto classDef = vo->getClassDefinition();
            if (!classDef) return;
            const auto& names = classDef->getFieldIndexToName();
            const size_t fieldCount = vo->getFieldCount();
            for (size_t i = 0; i < names.size() && i < fieldCount; ++i)
            {
                try
                {
                    children.push_back(valueToDebugVariable(names[i], vo->getFieldByIndex(i)));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::collectValueObjectChildren - field '"
                              << names[i] << "': " << e.what() << "\n";
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::collectValueObjectChildren - access: "
                      << e.what() << "\n";
        }
    }
} // namespace debugger
