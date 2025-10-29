#pragma once
#include "VariableInspector.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <memory>
#include <vector>

namespace debugger
{
    /**
     * Variable inspector specialized for bytecode VM execution
     *
     * Extracts variable information from the VM's internal stack
     * and call frames during bytecode execution.
     */
    class VMVariableInspector
    {
    public:
        VMVariableInspector();
        ~VMVariableInspector() = default;

        /**
         * Get local variables from the VM's current call frame
         *
         * @param vm The virtual machine instance
         * @return Vector of debug variables
         */
        std::vector<DebugVariable> getLocalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        /**
         * Get global variables from the VM's environment
         *
         * @param vm The virtual machine instance
         * @return Vector of debug variables
         */
        std::vector<DebugVariable> getGlobalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        /**
         * Get static class variables from all registered classes
         *
         * @param vm The virtual machine instance
         * @return Vector of debug variables (formatted as ClassName::fieldName)
         */
        std::vector<DebugVariable> getStaticVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        /**
         * Get children of an expandable variable (arrays, objects)
         *
         * @param vm The virtual machine instance
         * @param refId Reference ID of the parent variable
         * @return Vector of child debug variables
         */
        std::vector<DebugVariable> getVariableChildren(std::shared_ptr<vm::runtime::VirtualMachine> vm, int refId);

        /**
         * Clear cached variable references
         * Should be called when starting a new variables request to avoid stale refIds
         */
        void clearCache();

    private:
        // Helper to convert a Value to a debug variable
        DebugVariable valueToDebugVariable(const std::string& name, const value::Value& val);

        // Helper to get a string representation of a value
        std::string valueToString(const value::Value& val);

        // Helper to get the type name of a value
        std::string getTypeName(const value::Value& val);

        // Reference ID management for expandable variables
        int nextRefId;
        std::unordered_map<int, value::Value> refIdToValue;
    };
}
