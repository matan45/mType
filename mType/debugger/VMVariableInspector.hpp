#pragma once
#include "VariableInspector.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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
        std::vector<DebugVariable> getVariableChildren(std::shared_ptr<vm::runtime::VirtualMachine> vm, int64_t refId);

        /**
         * Resolve a raw VM value by debugger-visible name.
         * Searches the current frame, globals, and ClassName::staticField.
         */
        std::optional<value::Value> findVariableValue(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                                                      const std::string& name);

        /**
         * Format a raw VM value using the same cache/reference IDs as variables.
         */
        DebugVariable formatValue(const std::string& name, const value::Value& val);

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

        // Frame-classification helpers (promoted from an anonymous namespace
        // so the split _Find.cpp / _Collect.cpp translation units can share them).
        static bool isTopLevelScriptFrame(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                                          const vm::runtime::CallFrame& frame);
        static std::unordered_map<std::string, size_t> collectTopLevelLocalSlots(
            std::shared_ptr<vm::runtime::VirtualMachine> vm);

        // getLocalVariables per-frame-kind orchestrators (defined in
        // VMVariableInspector_Collect.cpp). Instance methods because they call
        // valueToDebugVariable which mutates refId state.
        void collectTopLevelScriptLocals(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                                         std::vector<DebugVariable>& variables);
        void collectLambdaFrameLocals(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                                      const vm::runtime::CallFrame& frame,
                                      std::vector<DebugVariable>& variables);
        void collectFunctionFrameLocals(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                                        const vm::runtime::CallFrame& frame,
                                        std::vector<DebugVariable>& variables);

        // Reference ID management for expandable variables
        // NOTE: Using int64_t to prevent overflow in long-running debug sessions
        // (int32 max ~2.1 billion could be reached; int64 max ~9 quintillion is safe)
        int64_t nextRefId;
        std::unordered_map<int64_t, value::Value> refIdToValue;
    };
}
