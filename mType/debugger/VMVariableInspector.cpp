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

    void VMVariableInspector::clearCache()
    {
        refIdToValue.clear();
        nextRefId = 1000;
    }

    std::vector<DebugVariable> VMVariableInspector::getLocalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        if (!vm)
        {
            return variables;
        }

        const auto& callStack = vm->getCallStack();
        if (callStack.empty())
        {
            return variables;
        }

        // Get the top call frame (current function)
        const auto& currentFrame = callStack.back();

        // Check if we have a shared frame (for lambdas)
        if (currentFrame.sharedFrame)
        {
            for (const auto& [name, slot] : currentFrame.sharedFrame->nameToSlot)
            {
                value::Value val = currentFrame.sharedFrame->getLocal(slot);
                variables.push_back(valueToDebugVariable(name, val));
            }
            return variables;
        }

        // Get function metadata for variable names
        const auto* program = vm->getProgram();
        if (!program)
        {
            return variables;
        }

        const auto* funcMetadata = program->getFunction(currentFrame.functionName);
        if (!funcMetadata)
        {
            return variables;
        }

        // Get the stack manager to access local variables
        auto stackManager = vm->getStackManager();
        if (!stackManager)
        {
            return variables;
        }

        const auto& stack = stackManager->getStack();

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
            for (size_t i = 0; i < funcMetadata->localVariableNames.size(); ++i)
            {
                size_t stackIndex = currentFrame.localBase + i;

                if (stackIndex < stack.size())
                {
                    const auto& val = stack[stackIndex];
                    // Only add if name is not empty
                    // Skip parameters (they're already added above)
                    if (!funcMetadata->localVariableNames[i].empty() && i >= funcMetadata->parameterCount)
                    {
                        // IMPORTANT: Only show variables that have been initialized (not std::monostate)
                        // This prevents showing variables that are declared later in the function
                        // but appear in localVariableNames (which captures all variables at compile time)
                        if (!std::holds_alternative<std::monostate>(val))
                        {
                            variables.push_back(valueToDebugVariable(funcMetadata->localVariableNames[i], val));
                        }
                    }
                }
            }
        }
        else
        {
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

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getGlobalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        if (!vm)
        {
            return variables;
        }

        // Get global variable metadata from the bytecode program
        const auto* program = vm->getProgram();
        if (!program)
        {
            return variables;
        }

        const auto& globalMetadata = program->getGlobalVariables();

        if (globalMetadata.empty())
        {
            return variables;
        }

        // Get the environment to access actual global variable values
        auto env = vm->getEnvironment();
        if (!env)
        {
            return variables;
        }

        // For each global variable in metadata, try to get its value
        for (const auto& meta : globalMetadata)
        {
            // Try to get the variable definition from the environment
            // Use the same method as VariableExecutor::handleLoadVar
            auto varDef = env->findVariable(meta.name);
            if (varDef)
            {
                // Get the value from the variable definition
                value::Value val = varDef->getValue();
                variables.push_back(valueToDebugVariable(meta.name, val));
            }
        }

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getStaticVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        if (!vm)
        {
            return variables;
        }

        // Get the environment to access the class registry
        auto env = vm->getEnvironment();
        if (!env)
        {
            return variables;
        }

        // Get the class registry
        auto classRegistry = env->getClassRegistry();
        if (!classRegistry)
        {
            return variables;
        }

        // Get all registered class names
        std::vector<std::string> classNames = classRegistry->getAllItemNames();

        // Iterate through all classes and their static fields
        for (const auto& className : classNames)
        {
            auto classDef = classRegistry->findClass(className);
            if (!classDef)
            {
                continue;
            }

            // Get static fields for this class
            const auto& staticFields = classDef->getStaticFields();
            for (const auto& [fieldName, fieldDef] : staticFields)
            {
                if (!fieldDef)
                {
                    continue;
                }

                // Format as ClassName::fieldName
                std::string qualifiedName = className + "::" + fieldName;

                // Get the value from the field definition
                value::Value val = fieldDef->getValue();
                variables.push_back(valueToDebugVariable(qualifiedName, val));
            }
        }

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
