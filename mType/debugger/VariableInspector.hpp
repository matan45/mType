#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <unordered_map>
#include "../environment/Environment.hpp"
#include "../value/ValueType.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/arrays/object/FlatMultiObjectArray.hpp"

namespace debugger {

    /**
     * Represents a variable for debugging display
     */
    struct DebugVariable {
        std::string name;
        std::string value;      // String representation of the value
        std::string type;       // Type name (int, string, MyClass, etc.)
        bool isExpandable;      // True for objects/arrays
        int referenceId;        // Non-zero for expandable objects
        std::vector<DebugVariable> children; // For expanded objects

        DebugVariable(const std::string& n, const std::string& v, const std::string& t,
                     bool expandable = false, int refId = 0)
            : name(n), value(v), type(t), isExpandable(expandable), referenceId(refId) {}
    };

    /**
     * Inspects and serializes variables from the mType environment
     *
     * This class provides methods to convert runtime values into
     * debug-friendly representations for display in the debugger.
     */
    class VariableInspector {
    private:
        int nextReferenceId;

        // Cache of object references for expansion
        std::unordered_map<int, std::shared_ptr<runtimeTypes::klass::ObjectInstance>> objectReferences;

        // Cache of array references for expansion (one map per array type)
        std::unordered_map<int, std::shared_ptr<value::NativeArray>> nativeArrayReferences;
        std::unordered_map<int, std::shared_ptr<value::FlatMultiArray>> flatMultiArrayReferences;
        std::unordered_map<int, std::shared_ptr<value::SparseMultiArray>> sparseMultiArrayReferences;
        std::unordered_map<int, std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>> flatMultiObjectArrayReferences;

    public:
        VariableInspector();

        /**
         * Get all local variables from the current scope
         */
        std::vector<DebugVariable> getLocalVariables(
            std::shared_ptr<environment::Environment> env
        );

        /**
         * Get all global variables
         */
        std::vector<DebugVariable> getGlobalVariables(
            std::shared_ptr<environment::Environment> env
        );

        /**
         * Get children of an expandable variable by reference ID
         */
        std::vector<DebugVariable> getVariableChildren(int referenceId);

        /**
         * Format a value as a debug variable
         */
        DebugVariable formatValue(
            const std::string& name,
            const value::Value& val
        );

        /**
         * Clear reference cache
         */
        void clearReferences();

    private:
        /**
         * Get string representation of a value
         */
        std::string valueToString(const value::Value& val);

        /**
         * Get type name of a value
         */
        std::string getTypeName(const value::Value& val);

        /**
         * Check if value is expandable (object or array)
         */
        bool isExpandable(const value::Value& val);

        /**
         * Store an object reference and return reference ID
         */
        int storeObjectReference(std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj);

        /**
         * Store array references and return reference ID
         */
        int storeNativeArrayReference(std::shared_ptr<value::NativeArray> arr);
        int storeFlatMultiArrayReference(std::shared_ptr<value::FlatMultiArray> arr);
        int storeSparseMultiArrayReference(std::shared_ptr<value::SparseMultiArray> arr);
        int storeFlatMultiObjectArrayReference(std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr);

        /**
         * Format object instance for display
         */
        DebugVariable formatObjectInstance(
            const std::string& name,
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj
        );

        /**
         * Format array types for display
         */
        DebugVariable formatNativeArray(
            const std::string& name,
            std::shared_ptr<value::NativeArray> arr
        );

        DebugVariable formatFlatMultiArray(
            const std::string& name,
            std::shared_ptr<value::FlatMultiArray> arr
        );

        DebugVariable formatSparseMultiArray(
            const std::string& name,
            std::shared_ptr<value::SparseMultiArray> arr
        );

        DebugVariable formatFlatMultiObjectArray(
            const std::string& name,
            std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr
        );

        /**
         * Get fields from an object instance
         */
        std::vector<DebugVariable> getObjectFields(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> obj
        );

        /**
         * Get elements from array types
         */
        std::vector<DebugVariable> getNativeArrayElements(
            std::shared_ptr<value::NativeArray> arr
        );

        std::vector<DebugVariable> getFlatMultiArrayElements(
            std::shared_ptr<value::FlatMultiArray> arr
        );

        std::vector<DebugVariable> getSparseMultiArrayElements(
            std::shared_ptr<value::SparseMultiArray> arr
        );

        std::vector<DebugVariable> getFlatMultiObjectArrayElements(
            std::shared_ptr<mType::value::arrays::FlatMultiObjectArray> arr
        );
    };

} // namespace debugger
