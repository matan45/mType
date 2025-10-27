#include "VariableInspector.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include <variant>
#include <iomanip>

namespace debugger {

    VariableInspector::VariableInspector()
        : nextReferenceId(1) {
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

        // Get all variables in current scope
        auto varManager = env->getVariableManager();
        if (!varManager) {
            return variables;
        }

        // Get variables from the current scope
        // Note: This depends on how VariableManager exposes scope variables
        // For now, we'll get variables by trying common names
        // TODO: Add proper API to VariableManager for enumerating scope variables

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

        // TODO: Add proper API to VariableManager for enumerating global variables
        // For now, return empty - will be populated when we have the API

        return variables;
    }

    std::vector<DebugVariable> VariableInspector::getVariableChildren(int referenceId) {
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

        using namespace value;

        // Check if it's an object instance
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            return formatObjectInstance(name, obj);
        }

        // Check if it's a NativeArray
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(val)) {
            auto arr = std::get<std::shared_ptr<NativeArray>>(val);
            return formatNativeArray(name, arr);
        }

        // Check if it's a FlatMultiArray
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(val)) {
            auto arr = std::get<std::shared_ptr<FlatMultiArray>>(val);
            return formatFlatMultiArray(name, arr);
        }

        // Check if it's a SparseMultiArray
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(val)) {
            auto arr = std::get<std::shared_ptr<SparseMultiArray>>(val);
            return formatSparseMultiArray(name, arr);
        }

        // Check if it's a FlatMultiObjectArray
        if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val)) {
            auto arr = std::get<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val);
            return formatFlatMultiObjectArray(name, arr);
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
        nextReferenceId = 1;
    }

    std::string VariableInspector::valueToString(const value::Value& val) {
        using namespace value;

        if (std::holds_alternative<int>(val)) {
            return std::to_string(std::get<int>(val));
        }
        if (std::holds_alternative<float>(val)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << std::get<float>(val);
            return ss.str();
        }
        if (std::holds_alternative<std::string>(val)) {
            return "\"" + std::get<std::string>(val) + "\"";
        }
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj) {
                return "{" + obj->getTypeName() + "}";
            }
            return "{null}";
        }

        // Handle arrays
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(val)) {
            auto arr = std::get<std::shared_ptr<NativeArray>>(val);
            if (arr) {
                return "[" + std::to_string(arr->size()) + " elements]";
            }
            return "[null]";
        }
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(val)) {
            auto arr = std::get<std::shared_ptr<FlatMultiArray>>(val);
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
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(val)) {
            auto arr = std::get<std::shared_ptr<SparseMultiArray>>(val);
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
        if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val)) {
            auto arr = std::get<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val);
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

        if (std::holds_alternative<std::monostate>(val)) {
            return "null";
        }

        return "<unknown>";
    }

    std::string VariableInspector::getTypeName(const value::Value& val) {
        using namespace value;

        if (std::holds_alternative<int>(val)) {
            return "int";
        }
        if (std::holds_alternative<float>(val)) {
            return "float";
        }
        if (std::holds_alternative<std::string>(val)) {
            return "string";
        }
        if (std::holds_alternative<bool>(val)) {
            return "bool";
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj) {
                return obj->getTypeName();
            }
            return "object";
        }

        // Array types
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(val)) {
            auto arr = std::get<std::shared_ptr<NativeArray>>(val);
            if (arr) {
                std::string elemType = arr->getElementTypeName();
                if (elemType.empty()) {
                    return "array";
                }
                return elemType + "[]";
            }
            return "array";
        }
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(val)) {
            return "array[multi]";
        }
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(val)) {
            return "array[sparse]";
        }
        if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val)) {
            return "array[objects]";
        }

        if (std::holds_alternative<std::monostate>(val)) {
            return "null";
        }

        return "unknown";
    }

    bool VariableInspector::isExpandable(const value::Value& val) {
        using namespace value;

        // Objects are expandable
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            return true;
        }

        // Arrays are expandable
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(val)) {
            return true;
        }
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(val)) {
            return true;
        }
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(val)) {
            return true;
        }
        if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val)) {
            return true;
        }

        return false;
    }

    int VariableInspector::storeObjectReference(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj) {

        int refId = nextReferenceId++;
        objectReferences[refId] = obj;
        return refId;
    }

    int VariableInspector::storeNativeArrayReference(std::shared_ptr<value::NativeArray> arr) {
        int refId = nextReferenceId++;
        nativeArrayReferences[refId] = arr;
        return refId;
    }

    int VariableInspector::storeFlatMultiArrayReference(std::shared_ptr<value::FlatMultiArray> arr) {
        int refId = nextReferenceId++;
        flatMultiArrayReferences[refId] = arr;
        return refId;
    }

    int VariableInspector::storeSparseMultiArrayReference(std::shared_ptr<value::SparseMultiArray> arr) {
        int refId = nextReferenceId++;
        sparseMultiArrayReferences[refId] = arr;
        return refId;
    }

    int VariableInspector::storeFlatMultiObjectArrayReference(
        std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr) {
        int refId = nextReferenceId++;
        flatMultiObjectArrayReferences[refId] = arr;
        return refId;
    }

    DebugVariable VariableInspector::formatObjectInstance(
        const std::string& name,
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj) {

        if (!obj) {
            return DebugVariable(name, "null", "object", false, 0);
        }

        int refId = storeObjectReference(obj);
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

        int refId = storeNativeArrayReference(arr);
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

        int refId = storeFlatMultiArrayReference(arr);

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

        int refId = storeSparseMultiArrayReference(arr);

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

        int refId = storeFlatMultiObjectArrayReference(arr);

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
        const auto& fieldMap = obj->getAllFieldValues();

        for (const auto& [fieldName, fieldValue] : fieldMap) {
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
        // Limit to first 100 elements to avoid overwhelming the debugger
        size_t displayCount = std::min(size, static_cast<size_t>(100));

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
        size_t displayCount = std::min(firstDimSize, static_cast<size_t>(100));

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

        // For sparse arrays, we can't easily enumerate all elements
        // Just show info about dimensions
        std::string dimInfo = "Dimensions: ";
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) dimInfo += " x ";
            dimInfo += std::to_string(dims[i]);
        }

        elements.push_back(DebugVariable(
            "[info]",
            dimInfo,
            "info",
            false,
            0
        ));

        // TODO: In the future, we could show only non-default values
        // This would require additional API from SparseMultiArray

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
        size_t displayCount = std::min(firstDimSize, static_cast<size_t>(100));

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
