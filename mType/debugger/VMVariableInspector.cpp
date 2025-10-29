#include "VMVariableInspector.hpp"
#include "../vm/runtime/stack/StackManager.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/LambdaValue.hpp"
#include "../value/StringPool.hpp"
#include "../environment/manager/VariableManager.hpp"
#include <iostream>
#include <sstream>
#include <variant>

namespace debugger
{
    VMVariableInspector::VMVariableInspector()
        : nextRefId(1000) // Start at 1000 to avoid confusion with scope handles
    {
    }

    std::vector<DebugVariable> VMVariableInspector::getLocalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;
        std::cerr << "[DEBUG VMInspector] getLocalVariables called" << std::endl;

        if (!vm)
        {
            std::cerr << "[DEBUG VMInspector] ERROR: VM is null" << std::endl;
            return variables;
        }

        const auto& callStack = vm->getCallStack();
        std::cerr << "[DEBUG VMInspector] Call stack size: " << callStack.size() << std::endl;
        if (callStack.empty())
        {
            std::cerr << "[DEBUG VMInspector] ERROR: Call stack is empty" << std::endl;
            return variables;
        }

        // Get the top call frame (current function)
        const auto& currentFrame = callStack.back();
        std::cerr << "[DEBUG VMInspector] Current function: " << currentFrame.functionName << std::endl;

        // Check if we have a shared frame (for lambdas)
        if (currentFrame.sharedFrame)
        {
            std::cerr << "[DEBUG VMInspector] Using shared frame (lambda)" << std::endl;
            for (const auto& [name, slot] : currentFrame.sharedFrame->nameToSlot)
            {
                value::Value val = currentFrame.sharedFrame->getLocal(slot);
                variables.push_back(valueToDebugVariable(name, val));
            }
            std::cerr << "[DEBUG VMInspector] Found " << variables.size() << " lambda variables" << std::endl;
            return variables;
        }

        // Get function metadata for variable names
        const auto* program = vm->getProgram();
        if (!program)
        {
            std::cerr << "[DEBUG VMInspector] ERROR: Program is null" << std::endl;
            return variables;
        }

        const auto* funcMetadata = program->getFunction(currentFrame.functionName);
        if (!funcMetadata)
        {
            std::cerr << "[DEBUG VMInspector] ERROR: No metadata for function: " << currentFrame.functionName << std::endl;
            return variables;
        }
        std::cerr << "[DEBUG VMInspector] Function metadata found" << std::endl;
        std::cerr << "[DEBUG VMInspector]   parameterCount: " << funcMetadata->parameterCount << std::endl;
        std::cerr << "[DEBUG VMInspector]   localCount: " << funcMetadata->localCount << std::endl;
        std::cerr << "[DEBUG VMInspector]   localVariableNames.size(): " << funcMetadata->localVariableNames.size() << std::endl;

        // Get the stack manager to access local variables
        auto stackManager = vm->getStackManager();
        if (!stackManager)
        {
            std::cerr << "[DEBUG VMInspector] ERROR: StackManager is null" << std::endl;
            return variables;
        }

        const auto& stack = stackManager->getStack();
        std::cerr << "[DEBUG VMInspector] Stack size: " << stack.size() << std::endl;
        std::cerr << "[DEBUG VMInspector] Current frame localBase: " << currentFrame.localBase << std::endl;

        // First, add parameters (we have names for these)
        for (size_t i = 0; i < funcMetadata->parameterCount && i < funcMetadata->parameterNames.size(); ++i)
        {
            size_t stackIndex = currentFrame.localBase + i;

            if (stackIndex < stack.size())
            {
                const auto& val = stack[stackIndex];
                variables.push_back(valueToDebugVariable(funcMetadata->parameterNames[i], val));
            }
        }

        // Add other local variables if we have names for them
        if (!funcMetadata->localVariableNames.empty())
        {
            std::cerr << "[DEBUG VMInspector] Using localVariableNames from metadata" << std::endl;
            for (size_t i = 0; i < funcMetadata->localVariableNames.size(); ++i)
            {
                size_t stackIndex = currentFrame.localBase + i;
                std::cerr << "[DEBUG VMInspector]   Checking var[" << i << "] stackIndex=" << stackIndex
                          << " name='" << funcMetadata->localVariableNames[i] << "'" << std::endl;

                if (stackIndex < stack.size())
                {
                    const auto& val = stack[stackIndex];
                    // Only add if name is not empty
                    if (!funcMetadata->localVariableNames[i].empty())
                    {
                        std::cerr << "[DEBUG VMInspector]   Adding variable: " << funcMetadata->localVariableNames[i] << std::endl;
                        variables.push_back(valueToDebugVariable(funcMetadata->localVariableNames[i], val));
                    }
                    else
                    {
                        std::cerr << "[DEBUG VMInspector]   Skipping variable with empty name" << std::endl;
                    }
                }
                else
                {
                    std::cerr << "[DEBUG VMInspector]   ERROR: stackIndex " << stackIndex << " >= stack.size() " << stack.size() << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "[DEBUG VMInspector] No localVariableNames, using generic names" << std::endl;
            // For now, add unnamed locals as "local_N"
            for (size_t i = funcMetadata->parameterCount; i < funcMetadata->localCount; ++i)
            {
                size_t stackIndex = currentFrame.localBase + i;
                std::string varName = "local_" + std::to_string(i);

                if (stackIndex < stack.size())
                {
                    const auto& val = stack[stackIndex];
                    variables.push_back(valueToDebugVariable(varName, val));
                }
            }
        }

        std::cerr << "[DEBUG VMInspector] Returning " << variables.size() << " variables" << std::endl;
        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getGlobalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        if (!vm)
        {
            return variables;
        }

        // Get global variables from the environment
        auto env = vm->getEnvironment();
        if (!env)
        {
            return variables;
        }

        // Access global scope through the variable manager
        // Note: In bytecode mode, global variables may not be used as much,
        // but we'll try to get them from the environment's variable manager
        auto varManager = env->getVariableManager();
        if (!varManager)
        {
            return variables;
        }

        // For now, we'll return empty since globals are typically not used in bytecode mode
        // Most variables are local on the VM stack
        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getVariableChildren(
        std::shared_ptr<vm::runtime::VirtualMachine> vm, int refId)
    {
        std::vector<DebugVariable> children;

        auto it = refIdToValue.find(refId);
        if (it == refIdToValue.end())
        {
            return children;
        }

        const auto& val = it->second;

        // Handle arrays
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val))
        {
            auto arr = std::get<std::shared_ptr<value::NativeArray>>(val);
            if (arr)
            {
                for (size_t i = 0; i < arr->size() && i < 100; ++i) // Limit to 100 elements
                {
                    std::string indexName = "[" + std::to_string(i) + "]";
                    value::Value element = arr->get(i);
                    children.push_back(valueToDebugVariable(indexName, element));
                }
            }
        }
        // Handle objects
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
        {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj)
            {
                const auto& fields = obj->getAllFieldValues();
                for (const auto& [fieldName, fieldValue] : fields)
                {
                    children.push_back(valueToDebugVariable(fieldName, fieldValue));
                }
            }
        }

        return children;
    }

    DebugVariable VMVariableInspector::valueToDebugVariable(const std::string& name, const value::Value& val)
    {
        // Check if this is an expandable type
        bool expandable = std::holds_alternative<std::shared_ptr<value::NativeArray>>(val) ||
                         std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

        int refId = 0;
        if (expandable)
        {
            refId = nextRefId++;
            refIdToValue[refId] = val;
        }

        // Create DebugVariable using constructor
        DebugVariable var(name, valueToString(val), getTypeName(val), expandable, refId);
        return var;
    }

    std::string VMVariableInspector::valueToString(const value::Value& val)
    {
        if (std::holds_alternative<std::monostate>(val))
        {
            return "null";
        }
        else if (std::holds_alternative<int>(val))
        {
            return std::to_string(std::get<int>(val));
        }
        else if (std::holds_alternative<float>(val))
        {
            return std::to_string(std::get<float>(val));
        }
        else if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val) ? "true" : "false";
        }
        else if (std::holds_alternative<std::string>(val))
        {
            return "\"" + std::get<std::string>(val) + "\"";
        }
        else if (std::holds_alternative<value::InternedString>(val))
        {
            auto internedStr = std::get<value::InternedString>(val);
            return "\"" + internedStr.getString() + "\"";
        }
        else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val))
        {
            auto arr = std::get<std::shared_ptr<value::NativeArray>>(val);
            if (arr)
            {
                return "Array[" + std::to_string(arr->size()) + "]";
            }
            return "Array[null]";
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
        {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj)
            {
                return obj->getTypeName() + " instance";
            }
            return "Object[null]";
        }
        else if (std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(val))
        {
            return "<lambda>";
        }
        else if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(val))
        {
            return "<lambda>";
        }
        else if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(val))
        {
            return "<multi-array>";
        }
        else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(val))
        {
            return "<sparse-array>";
        }
        else if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val))
        {
            return "<object-array>";
        }
        else if (std::holds_alternative<std::shared_ptr<value::PromiseValue>>(val))
        {
            return "<promise>";
        }
        else if (std::holds_alternative<std::nullptr_t>(val))
        {
            return "null";
        }
        else
        {
            return "<unknown>";
        }
    }

    std::string VMVariableInspector::getTypeName(const value::Value& val)
    {
        if (std::holds_alternative<std::monostate>(val))
        {
            return "null";
        }
        else if (std::holds_alternative<int>(val))
        {
            return "Int";
        }
        else if (std::holds_alternative<float>(val))
        {
            return "Float";
        }
        else if (std::holds_alternative<bool>(val))
        {
            return "Bool";
        }
        else if (std::holds_alternative<std::string>(val))
        {
            return "String";
        }
        else if (std::holds_alternative<value::InternedString>(val))
        {
            return "String";
        }
        else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val))
        {
            return "Array";
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
        {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj)
            {
                return obj->getTypeName();
            }
            return "Object";
        }
        else if (std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(val))
        {
            return "Lambda";
        }
        else if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(val))
        {
            return "Lambda";
        }
        else if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(val))
        {
            return "MultiArray";
        }
        else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(val))
        {
            return "SparseArray";
        }
        else if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(val))
        {
            return "ObjectArray";
        }
        else if (std::holds_alternative<std::shared_ptr<value::PromiseValue>>(val))
        {
            return "Promise";
        }
        else if (std::holds_alternative<std::nullptr_t>(val))
        {
            return "null";
        }
        else
        {
            return "Unknown";
        }
    }
}
