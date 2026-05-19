#include "VariableInspector.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "DebuggerConstants.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../value/ValueShim.hpp"

namespace debugger {

    VariableInspector::VariableInspector()
        : nextReferenceId(constants::INITIAL_REFERENCE_ID) {
    }

    std::vector<DebugVariable> VariableInspector::getLocalVariables(
        std::shared_ptr<environment::Environment> env) {

        std::vector<DebugVariable> variables;

        if (!env) {
            return variables;
        }

        auto scopeManager = env->getScopeManager();
        if (!scopeManager) {
            return variables;
        }

        auto currentScope = scopeManager->getCurrentScope();
        if (!currentScope) {
            return variables;
        }

        // Current scope only — do not walk into parent scopes.
        std::vector<std::string> varNames = currentScope->getAllVariableNames();

        for (const auto& varName : varNames) {
            auto varDef = currentScope->findVariableInCurrentScope(varName);
            if (varDef) {
                variables.push_back(formatValue(varName, varDef->getValue()));
            }
        }

        return variables;
    }

    std::vector<DebugVariable> VariableInspector::getGlobalVariables(
        std::shared_ptr<environment::Environment> env) {

        std::vector<DebugVariable> variables;

        if (!env) {
            return variables;
        }

        auto varManager = env->getVariableManager();
        if (!varManager) {
            return variables;
        }

        std::vector<std::string> varNames = varManager->getAllVariableNames();

        for (const auto& varName : varNames) {
            auto varDef = varManager->findVariable(varName);
            if (varDef) {
                variables.push_back(formatValue(varName, varDef->getValue()));
            }
        }

        return variables;
    }

    std::vector<DebugVariable> VariableInspector::getVariableChildren(int64_t referenceId) {
        auto objIt = objectReferences.find(referenceId);
        if (objIt != objectReferences.end()) {
            return getObjectFields(objIt->second);
        }

        auto nativeArrIt = nativeArrayReferences.find(referenceId);
        if (nativeArrIt != nativeArrayReferences.end()) {
            return getNativeArrayElements(nativeArrIt->second);
        }

        auto flatMultiArrIt = flatMultiArrayReferences.find(referenceId);
        if (flatMultiArrIt != flatMultiArrayReferences.end()) {
            return getFlatMultiArrayElements(flatMultiArrIt->second);
        }

        auto sparseArrIt = sparseMultiArrayReferences.find(referenceId);
        if (sparseArrIt != sparseMultiArrayReferences.end()) {
            return getSparseMultiArrayElements(sparseArrIt->second);
        }

        auto flatMultiObjArrIt = flatMultiObjectArrayReferences.find(referenceId);
        if (flatMultiObjArrIt != flatMultiObjectArrayReferences.end()) {
            return getFlatMultiObjectArrayElements(flatMultiObjArrIt->second);
        }

        return std::vector<DebugVariable>();
    }

    DebugVariable VariableInspector::formatValue(
        const std::string& name,
        const value::Value& val) {

        if (value::isObject(val)) {
            return formatObjectInstance(name, value::asObject(val));
        }
        if (value::isNativeArray(val)) {
            return formatNativeArray(name, value::asNativeArray(val));
        }
        if (value::isFlatMultiArray(val)) {
            return formatFlatMultiArray(name, value::asFlatMultiArray(val));
        }
        if (value::isSparseMultiArray(val)) {
            return formatSparseMultiArray(name, value::asSparseMultiArray(val));
        }
        if (value::isFlatMultiObjectArray(val)) {
            return formatFlatMultiObjectArray(name, value::asFlatMultiObjectArray(val));
        }

        std::string valueStr = valueToString(val);
        std::string typeName = getTypeName(val);
        bool expandable = isExpandable(val);

        return DebugVariable(name, valueStr, typeName, expandable, 0);
    }

    void VariableInspector::clearReferences() {
        objectReferences.clear();
        nativeArrayReferences.clear();
        flatMultiArrayReferences.clear();
        sparseMultiArrayReferences.clear();
        flatMultiObjectArrayReferences.clear();
        nextReferenceId = constants::INITIAL_REFERENCE_ID;
    }

    std::string VariableInspector::valueToString(const value::Value& val) {
        if (value::isInt(val)) {
            return std::to_string(value::asInt(val));
        }
        if (value::isFloat(val)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << value::asFloat(val);
            return ss.str();
        }
        // MYT-317: SSO-aware. Folds all three string forms into one branch.
        if (value::isAnyString(val)) {
            return "\"" + std::string(value::asStringView(val)) + "\"";
        }
        if (value::isBool(val)) {
            return value::asBool(val) ? "true" : "false";
        }
        if (value::isObject(val)) {
            auto obj = value::asObject(val);
            if (obj) {
                return "{" + obj->getTypeName() + "}";
            }
            return "{null}";
        }

        if (value::isNativeArray(val)) {
            auto arr = value::asNativeArray(val);
            if (arr) {
                return "[" + std::to_string(arr->size()) + " elements]";
            }
            return "[null]";
        }
        if (value::isFlatMultiArray(val)) {
            auto arr = value::asFlatMultiArray(val);
            if (arr) {
                auto dims = arr->getDimensions();
                std::string dimStr;
                for (size_t i = 0; i < dims.size(); ++i) {
                    if (i > 0) dimStr += "x";
                    dimStr += std::to_string(dims[i]);
                }
                return "[" + dimStr + "]";
            }
            return "[null]";
        }
        if (value::isSparseMultiArray(val)) {
            auto arr = value::asSparseMultiArray(val);
            if (arr) {
                auto dims = arr->getDimensions();
                std::string dimStr;
                for (size_t i = 0; i < dims.size(); ++i) {
                    if (i > 0) dimStr += "x";
                    dimStr += std::to_string(dims[i]);
                }
                return "[" + dimStr + " sparse]";
            }
            return "[null]";
        }
        if (value::isFlatMultiObjectArray(val)) {
            auto arr = value::asFlatMultiObjectArray(val);
            if (arr) {
                auto dims = arr->getDimensions();
                std::string dimStr;
                for (size_t i = 0; i < dims.size(); ++i) {
                    if (i > 0) dimStr += "x";
                    dimStr += std::to_string(dims[i]);
                }
                return "[" + dimStr + " objects]";
            }
            return "[null]";
        }

        if (value::isVoid(val) || value::isNullType(val)) {
            return "null";
        }

        return "<unknown>";
    }

    std::string VariableInspector::getTypeName(const value::Value& val) {
        if (value::isInt(val))           return "int";
        if (value::isFloat(val))         return "float";
        // MYT-317: STRING_INLINE also reports as "string".
        if (value::isAnyString(val))     return "string";
        if (value::isBool(val))          return "bool";
        if (value::isObject(val)) {
            auto obj = value::asObject(val);
            if (obj) {
                return obj->getTypeName();
            }
            return "object";
        }

        if (value::isNativeArray(val)) {
            auto arr = value::asNativeArray(val);
            if (arr) {
                std::string elemType = arr->getElementTypeName();
                if (elemType.empty()) {
                    return "array";
                }
                return elemType + "[]";
            }
            return "array";
        }
        if (value::isFlatMultiArray(val))       return "array[multi]";
        if (value::isSparseMultiArray(val))     return "array[sparse]";
        if (value::isFlatMultiObjectArray(val)) return "array[objects]";

        if (value::isVoid(val) || value::isNullType(val)) {
            return "null";
        }

        return "unknown";
    }

    bool VariableInspector::isExpandable(const value::Value& val) {
        return value::isObject(val) ||
               value::isNativeArray(val) ||
               value::isFlatMultiArray(val) ||
               value::isSparseMultiArray(val) ||
               value::isFlatMultiObjectArray(val);
    }

    int64_t VariableInspector::storeObjectReference(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj) {

        int64_t refId = nextReferenceId++;
        objectReferences[refId] = obj;
        return refId;
    }

    int64_t VariableInspector::storeNativeArrayReference(std::shared_ptr<value::NativeArray> arr) {
        int64_t refId = nextReferenceId++;
        nativeArrayReferences[refId] = arr;
        return refId;
    }

    int64_t VariableInspector::storeFlatMultiArrayReference(std::shared_ptr<value::FlatMultiArray> arr) {
        int64_t refId = nextReferenceId++;
        flatMultiArrayReferences[refId] = arr;
        return refId;
    }

    int64_t VariableInspector::storeSparseMultiArrayReference(std::shared_ptr<value::SparseMultiArray> arr) {
        int64_t refId = nextReferenceId++;
        sparseMultiArrayReferences[refId] = arr;
        return refId;
    }

    int64_t VariableInspector::storeFlatMultiObjectArrayReference(
        std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr) {
        int64_t refId = nextReferenceId++;
        flatMultiObjectArrayReferences[refId] = arr;
        return refId;
    }

}
