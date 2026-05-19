/**
 * VMVariableInspector — value formatting and child enumeration.
 *
 * Holds getVariableChildren, valueToString, and getTypeName. The latter
 * two are if-else chains over value-type tags; each branch group is
 * factored into a small `tryFormat*` helper that returns std::optional,
 * so the top-level methods stay under the 50-line cap.
 *
 * Driver methods (ctor / clearCache / formatValue / valueToDebugVariable)
 * live in VMVariableInspector.cpp.
 */

#include "VMVariableInspector.hpp"
#include <cstddef>
#include <iostream>
#include <sstream>
#include "DebuggerConstants.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"

namespace debugger
{
    namespace
    {
        std::optional<std::string> tryFormatNullOrPrimitive(const value::Value& val)
        {
            if (value::isVoid(val) || value::isNullType(val))
            {
                return std::string{"null"};
            }
            if (value::isInt(val))
            {
                return std::to_string(value::asInt(val));
            }
            if (value::isFloat(val))
            {
                return std::to_string(value::asFloat(val));
            }
            if (value::isBool(val))
            {
                return std::string{value::asBool(val) ? "true" : "false"};
            }
            return std::nullopt;
        }

        std::optional<std::string> tryFormatStringValue(const value::Value& val)
        {
            // MYT-317: SSO-aware. Folds all three string forms into one branch.
            if (!value::isAnyString(val))
            {
                return std::nullopt;
            }
            try
            {
                return "\"" + std::string(value::asStringView(val)) + "\"";
            }
            catch (const std::exception& e)
            {
                return "\"<error: " + std::string(e.what()) + ">\"";
            }
        }

        std::optional<std::string> tryFormatArrayValue(const value::Value& val)
        {
            if (!value::isNativeArray(val))
            {
                return std::nullopt;
            }
            auto arr = value::asNativeArray(val);
            if (!arr)
            {
                return std::string{"Array[null]"};
            }
            try
            {
                return "Array[" + std::to_string(arr->size()) + "]";
            }
            catch (const std::exception& e)
            {
                return "Array[error: " + std::string(e.what()) + "]";
            }
        }

        std::optional<std::string> tryFormatComposite(const value::Value& val)
        {
            if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (!obj)
                {
                    return std::string{"Object[null]"};
                }
                try
                {
                    return obj->getTypeName() + " instance";
                }
                catch (const std::exception& e)
                {
                    return "Object[error: " + std::string(e.what()) + "]";
                }
            }
            if (value::isLambda(val))
            {
                return std::string{"<lambda>"};
            }
            if (value::isFlatMultiArray(val))
            {
                return std::string{"<multi-array>"};
            }
            if (value::isSparseMultiArray(val))
            {
                return std::string{"<sparse-array>"};
            }
            if (value::isFlatMultiObjectArray(val))
            {
                return std::string{"<object-array>"};
            }
            if (value::isPromise(val) || value::isPromiseInt(val))
            {
                return std::string{"<promise>"};
            }
            return std::nullopt;
        }

        std::optional<std::string> tryGetPrimitiveTypeName(const value::Value& val)
        {
            if (value::isVoid(val) || value::isNullType(val))
            {
                return std::string{"null"};
            }
            if (value::isInt(val))
            {
                return std::string{"Int"};
            }
            if (value::isFloat(val))
            {
                return std::string{"Float"};
            }
            if (value::isBool(val))
            {
                return std::string{"Bool"};
            }
            // MYT-317: STRING_INLINE also reports as "String".
            if (value::isAnyString(val))
            {
                return std::string{"String"};
            }
            return std::nullopt;
        }

        std::optional<std::string> tryGetCollectionTypeName(const value::Value& val)
        {
            if (value::isNativeArray(val))
            {
                return std::string{"Array"};
            }
            if (value::isFlatMultiArray(val))
            {
                return std::string{"MultiArray"};
            }
            if (value::isSparseMultiArray(val))
            {
                return std::string{"SparseArray"};
            }
            if (value::isFlatMultiObjectArray(val))
            {
                return std::string{"ObjectArray"};
            }
            return std::nullopt;
        }

        std::optional<std::string> tryGetCompositeTypeName(const value::Value& val)
        {
            if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (!obj)
                {
                    return std::string{"Object"};
                }
                try
                {
                    return obj->getTypeName();
                }
                catch (const std::exception&)
                {
                    return std::string{"Object[error]"};
                }
            }
            if (value::isLambda(val))
            {
                return std::string{"Lambda"};
            }
            if (value::isPromise(val) || value::isPromiseInt(val))
            {
                return std::string{"Promise"};
            }
            return std::nullopt;
        }

        template <typename ToVar>
        void collectArrayChildren(const value::Value& val,
                                  ToVar&& toVar,
                                  std::vector<DebugVariable>& children)
        {
            auto arr = value::asNativeArray(val);
            if (!arr)
            {
                return;
            }
            try
            {
                size_t arrSize = arr->size();
                size_t limit = std::min(arrSize, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

                for (size_t i = 0; i < limit; ++i)
                {
                    try
                    {
                        std::string indexName = "[" + std::to_string(i) + "]";
                        value::Value element = arr->get(i);
                        children.push_back(toVar(indexName, element));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getVariableChildren() - Error getting array element "
                                  << i << ": " << e.what() << "\n";
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "VMVariableInspector::getVariableChildren() - Error accessing array: "
                          << e.what() << "\n";
            }
        }

        template <typename ToVar>
        void collectObjectChildren(const value::Value& val,
                                   ToVar&& toVar,
                                   std::vector<DebugVariable>& children)
        {
            auto* obj = value::asObjectInstanceRaw(val);
            if (!obj)
            {
                return;
            }
            try
            {
                const auto& fields = obj->getAllFields();
                for (const auto& [fieldName, fieldValue] : fields)
                {
                    try
                    {
                        children.push_back(toVar(fieldName, fieldValue));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getVariableChildren() - Error getting object field '"
                                  << fieldName << "': " << e.what() << "\n";
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "VMVariableInspector::getVariableChildren() - Error accessing object fields: "
                          << e.what() << "\n";
            }
        }
    } // namespace

    std::vector<DebugVariable> VMVariableInspector::getVariableChildren(
        std::shared_ptr<vm::runtime::VirtualMachine> vm, int64_t refId)
    {
        (void)vm;
        std::vector<DebugVariable> children;

        try
        {
            auto it = refIdToValue.find(refId);
            if (it == refIdToValue.end())
            {
                return children;
            }
            const auto& val = it->second;

            auto toVar = [this](const std::string& name, const value::Value& v) {
                return valueToDebugVariable(name, v);
            };

            if (value::isNativeArray(val))
            {
                collectArrayChildren(val, toVar, children);
            }
            else if (value::isAnyObject(val))
            {
                collectObjectChildren(val, toVar, children);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getVariableChildren() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getVariableChildren() - Unknown exception\n";
        }

        return children;
    }

    std::string VMVariableInspector::valueToString(const value::Value& val)
    {
        try
        {
            if (auto s = tryFormatNullOrPrimitive(val)) return *s;
            if (auto s = tryFormatStringValue(val)) return *s;
            if (auto s = tryFormatArrayValue(val)) return *s;
            if (auto s = tryFormatComposite(val)) return *s;
            return "<unknown>";
        }
        catch (const std::exception& e)
        {
            return "<error: " + std::string(e.what()) + ">";
        }
        catch (...)
        {
            return "<unknown error>";
        }
    }

    std::string VMVariableInspector::getTypeName(const value::Value& val)
    {
        try
        {
            if (auto n = tryGetPrimitiveTypeName(val)) return *n;
            if (auto n = tryGetCollectionTypeName(val)) return *n;
            if (auto n = tryGetCompositeTypeName(val)) return *n;
            return "Unknown";
        }
        catch (const std::exception&)
        {
            return "Error";
        }
        catch (...)
        {
            return "Unknown";
        }
    }
} // namespace debugger
