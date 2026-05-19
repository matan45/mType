#include "VariableInspector.hpp"
#include <algorithm>
#include <cstddef>
#include "DebuggerConstants.hpp"
#include "../value/ValueShim.hpp"

namespace debugger {

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
        // Cap display so a huge array doesn't overwhelm the debugger UI.
        size_t displayCount = std::min(size, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

        for (size_t i = 0; i < displayCount; ++i) {
            try {
                value::Value element = arr->get(i);
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

        if (size > displayCount) {
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

        // Show first-dimension elements only for multi-D arrays.
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

        auto stats = arr->getSparsityStats();

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

        if (stats.currentMode == value::SparseMultiArray::StorageMode::SPARSE &&
            stats.nonDefaultElements > 0) {

            // TODO: SparseMultiArray needs a public iterator over non-default
            // entries before this can list them; for now just report the count.
            elements.push_back(DebugVariable(
                "[mode]",
                "SPARSE mode with " + std::to_string(stats.nonDefaultElements) + " set values",
                "info",
                false,
                0
            ));
        } else if (stats.currentMode == value::SparseMultiArray::StorageMode::DENSE) {
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

        return elements;
    }

}
