#include "VariableInspector.hpp"
#include "DebuggerConstants.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include "../value/ValueShim.hpp"
#include <iomanip>
#include <iostream>

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

        // Get the scope manager
        auto scopeManager = env->getScopeManager();
        if (!scopeManager) {
            return variables;
        }

        // Get the current scope
        auto currentScope = scopeManager->getCurrentScope();
        if (!currentScope) {
            return variables;
        }

        // Get all variable names from current scope (not including parent scopes)
        std::vector<std::string> varNames = currentScope->getAllVariableNames();

        // For each variable, get its value and format it
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

        // Get global variable manager
        auto varManager = env->getVariableManager();
        if (!varManager) {
            return variables;
        }

        // Get all global variable names
        std::vector<std::string> varNames = varManager->getAllVariableNames();

        // For each variable, get its value and format it
        for (const auto& varName : varNames) {
            auto varDef = varManager->findVariable(varName);
            if (varDef) {
                variables.push_back(formatValue(varName, varDef->getValue()));
            }
        }

        return variables;
    }

    std::vector<DebugVariable> VariableInspector::getVariableChildren(int64_t referenceId) {
        // Check if it's an object reference
        auto objIt = objectReferences.find(referenceId);
        if (objIt != objectReferences.end()) {
            return getObjectFields(objIt->second);
        }

        // Check if it's a NativeArray reference
        auto nativeArrIt = nativeArrayReferences.find(referenceId);
        if (nativeArrIt != nativeArrayReferences.end()) {
            return getNativeArrayElements(nativeArrIt->second);
        }

        // Check if it's a FlatMultiArray reference
        auto flatMultiArrIt = flatMultiArrayReferences.find(referenceId);
        if (flatMultiArrIt != flatMultiArrayReferences.end()) {
            return getFlatMultiArrayElements(flatMultiArrIt->second);
        }

        // Check if it's a SparseMultiArray reference
        auto sparseArrIt = sparseMultiArrayReferences.find(referenceId);
        if (sparseArrIt != sparseMultiArrayReferences.end()) {
            return getSparseMultiArrayElements(sparseArrIt->second);
        }

        // Check if it's a FlatMultiObjectArray reference
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

        // Simple value
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
        if (value::isString(val)) {
            return "\"" + value::asString(val) + "\"";
        }
        if (value::isInternedString(val)) {
            return "\"" + value::asInternedString(val).getString() + "\"";
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

        // Handle arrays
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
        if (value::isString(val))        return "string";
        if (value::isInternedString(val)) return "string";
        if (value::isBool(val))          return "bool";
        if (value::isObject(val)) {
            auto obj = value::asObject(val);
            if (obj) {
                return obj->getTypeName();
            }
            return "object";
        }

        // Array types
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

    DebugVariable VariableInspector::formatObjectInstance(
        const std::string& name,
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj) {

        if (!obj) {
            return DebugVariable(name, "null", "object", false, 0);
        }

        int64_t refId = storeObjectReference(obj);
        std::string valueStr = "{" + obj->getTypeName() + "}";
        std::string typeName = obj->getTypeName();

        return DebugVariable(name, valueStr, typeName, true, refId);
    }

    DebugVariable VariableInspector::formatNativeArray(
        const std::string& name,
        std::shared_ptr<value::NativeArray> arr) {

        if (!arr) {
            return DebugVariable(name, "[null]", "array", false, 0);
        }

        int64_t refId = storeNativeArrayReference(arr);
        std::string valueStr = "[" + std::to_string(arr->size()) + " elements]";

        std::string elemType = arr->getElementTypeName();
        std::string typeName = elemType.empty() ? "array" : elemType + "[]";

        return DebugVariable(name, valueStr, typeName, true, refId);
    }

    DebugVariable VariableInspector::formatFlatMultiArray(
        const std::string& name,
        std::shared_ptr<value::FlatMultiArray> arr) {

        if (!arr) {
            return DebugVariable(name, "[null]", "array[multi]", false, 0);
        }

        int64_t refId = storeFlatMultiArrayReference(arr);

        auto dims = arr->getDimensions();
        std::string dimStr;
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) dimStr += "x";
            dimStr += std::to_string(dims[i]);
        }
        std::string valueStr = "[" + dimStr + "]";

        return DebugVariable(name, valueStr, "array[multi]", true, refId);
    }

    DebugVariable VariableInspector::formatSparseMultiArray(
        const std::string& name,
        std::shared_ptr<value::SparseMultiArray> arr) {

        if (!arr) {
            return DebugVariable(name, "[null]", "array[sparse]", false, 0);
        }

        int64_t refId = storeSparseMultiArrayReference(arr);

        auto dims = arr->getDimensions();
        std::string dimStr;
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) dimStr += "x";
            dimStr += std::to_string(dims[i]);
        }
        std::string valueStr = "[" + dimStr + " sparse]";

        return DebugVariable(name, valueStr, "array[sparse]", true, refId);
    }

    DebugVariable VariableInspector::formatFlatMultiObjectArray(
        const std::string& name,
        std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr) {

        if (!arr) {
            return DebugVariable(name, "[null]", "array[objects]", false, 0);
        }

        int64_t refId = storeFlatMultiObjectArrayReference(arr);

        auto dims = arr->getDimensions();
        std::string dimStr;
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) dimStr += "x";
            dimStr += std::to_string(dims[i]);
        }
        std::string valueStr = "[" + dimStr + " objects]";

        return DebugVariable(name, valueStr, "array[objects]", true, refId);
    }

    std::vector<DebugVariable> VariableInspector::getObjectFields(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj) {

        std::vector<DebugVariable> fields;

        if (!obj) {
            return fields;
        }

        // Get all fields from the object
        const auto& fieldList = obj->getAllFields();

        for (const auto& [fieldName, fieldValue] : fieldList) {
            fields.push_back(formatValue(fieldName, fieldValue));
        }

        return fields;
    }

    std::vector<DebugVariable> VariableInspector::getNativeArrayElements(
        std::shared_ptr<value::NativeArray> arr) {

        std::vector<DebugVariable> elements;

        if (!arr) {
            return elements;
        }

        size_t size = arr->size();
        // Limit array display to avoid overwhelming the debugger
        size_t displayCount = std::min(size, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

        for (size_t i = 0; i < displayCount; ++i) {
            try {
                value::Value element = arr->get(i);
                std::string indexName = "[" + std::to_string(i) + "]";
                elements.push_back(formatValue(indexName, element));
            } catch (...) {
                // If we can't get an element, show error
                elements.push_back(DebugVariable(
                    "[" + std::to_string(i) + "]",
                    "<error>",
                    "unknown",
                    false,
                    0
                ));
            }
        }

        if (size > displayCount) {
            // Add a note that more elements exist
            elements.push_back(DebugVariable(
                "[...]",
                "(" + std::to_string(size - displayCount) + " more elements)",
                "info",
                false,
                0
            ));
        }

        return elements;
    }

    std::vector<DebugVariable> VariableInspector::getFlatMultiArrayElements(
        std::shared_ptr<value::FlatMultiArray> arr) {

        std::vector<DebugVariable> elements;

        if (!arr) {
            return elements;
        }

        auto dims = arr->getDimensions();
        if (dims.empty()) {
            return elements;
        }

        // For multi-dimensional arrays, show first dimension elements
        size_t firstDimSize = dims[0];
        size_t displayCount = std::min(firstDimSize, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

        for (size_t i = 0; i < displayCount; ++i) {
            try {
                // Get element at first dimension index
                std::vector<size_t> indices(dims.size(), 0);
                indices[0] = i;
                value::Value element = arr->get(indices);

                std::string indexName = "[" + std::to_string(i) + "]";
                elements.push_back(formatValue(indexName, element));
            } catch (...) {
                elements.push_back(DebugVariable(
                    "[" + std::to_string(i) + "]",
                    "<error>",
                    "unknown",
                    false,
                    0
                ));
            }
        }

        if (firstDimSize > displayCount) {
            elements.push_back(DebugVariable(
                "[...]",
                "(" + std::to_string(firstDimSize - displayCount) + " more elements)",
                "info",
                false,
                0
            ));
        }

        return elements;
    }

    std::vector<DebugVariable> VariableInspector::getSparseMultiArrayElements(
        std::shared_ptr<value::SparseMultiArray> arr) {

        std::vector<DebugVariable> elements;

        if (!arr) {
            return elements;
        }

        auto dims = arr->getDimensions();
        if (dims.empty()) {
            return elements;
        }

        // Get sparsity statistics
        auto stats = arr->getSparsityStats();

        // Show sparsity information
        std::string dimInfo = "Dimensions: ";
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) dimInfo += " x ";
            dimInfo += std::to_string(dims[i]);
        }
        dimInfo += " | Non-default: " + std::to_string(stats.nonDefaultElements) +
                   " / " + std::to_string(stats.totalElements);

        elements.push_back(DebugVariable(
            "[sparsity]",
            dimInfo,
            "info",
            false,
            0
        ));

        // For sparse mode arrays, show non-default values
        if (stats.currentMode == value::SparseMultiArray::StorageMode::SPARSE &&
            stats.nonDefaultElements > 0) {

            // Limit non-default elements display
            size_t displayCount = std::min(stats.nonDefaultElements, constants::MAX_ARRAY_DISPLAY_ELEMENTS);
            size_t count = 0;

            // Note: We can't access the internal sparse map directly without adding a public API
            // For now, we'll just show that there are non-default elements
            // TODO: Add public API to SparseMultiArray to iterate over non-default values

            elements.push_back(DebugVariable(
                "[mode]",
                "SPARSE mode with " + std::to_string(stats.nonDefaultElements) + " set values",
                "info",
                false,
                0
            ));
        } else if (stats.currentMode == value::SparseMultiArray::StorageMode::DENSE) {
            // For dense mode, show first dimension elements like other arrays
            size_t firstDimSize = dims[0];
            size_t displayCount = std::min(firstDimSize, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

            for (size_t i = 0; i < displayCount; ++i) {
                try {
                    std::vector<size_t> indices(dims.size(), 0);
                    indices[0] = i;
                    value::Value element = arr->get(indices);

                    std::string indexName = "[" + std::to_string(i) + "]";
                    elements.push_back(formatValue(indexName, element));
                } catch (...) {
                    elements.push_back(DebugVariable(
                        "[" + std::to_string(i) + "]",
                        "<error>",
                        "unknown",
                        false,
                        0
                    ));
                }
            }

            if (firstDimSize > displayCount) {
                elements.push_back(DebugVariable(
                    "[...]",
                    "(" + std::to_string(firstDimSize - displayCount) + " more elements)",
                    "info",
                    false,
                    0
                ));
            }
        }

        return elements;
    }

    std::vector<DebugVariable> VariableInspector::getFlatMultiObjectArrayElements(
        std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr) {

        std::vector<DebugVariable> elements;

        if (!arr) {
            return elements;
        }

        auto dims = arr->getDimensions();
        if (dims.empty()) {
            return elements;
        }

        // For multi-dimensional object arrays, show first dimension elements
        size_t firstDimSize = dims[0];
        size_t displayCount = std::min(firstDimSize, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

        for (size_t i = 0; i < displayCount; ++i) {
            try {
                // Get element at first dimension index
                std::vector<size_t> indices(dims.size(), 0);
                indices[0] = i;
                value::Value element = arr->get(indices);

                std::string indexName = "[" + std::to_string(i) + "]";
                elements.push_back(formatValue(indexName, element));
            } catch (...) {
                elements.push_back(DebugVariable(
                    "[" + std::to_string(i) + "]",
                    "<error>",
                    "unknown",
                    false,
                    0
                ));
            }
        }

        if (firstDimSize > displayCount) {
            elements.push_back(DebugVariable(
                "[...]",
                "(" + std::to_string(firstDimSize - displayCount) + " more elements)",
                "info",
                false,
                0
            ));
        }

        return elements;
    }

} // namespace debugger
