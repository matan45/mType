#include "VMVariableInspector.hpp"
#include "DebuggerConstants.hpp"
#include "../vm/runtime/stack/StackManager.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/StringPool.hpp"
#include "../environment/manager/VariableManager.hpp"
#include <iostream>
#include <sstream>
#include <variant>

namespace debugger
{
    VMVariableInspector::VMVariableInspector()
        : nextRefId(constants::INITIAL_REFERENCE_ID)
    {
    }

    void VMVariableInspector::clearCache()
    {
        refIdToValue.clear();
        nextRefId = constants::INITIAL_REFERENCE_ID;
    }

    std::vector<DebugVariable> VMVariableInspector::getLocalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            // DEFENSIVE: Validate VM is not null
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - VM is null\n";
                return variables;
            }

            const auto& callStack = vm->getCallStack();
            if (callStack.empty())
            {
                // Not an error - just means no function is executing
                return variables;
            }

            // Get the top call frame (current function)
            const auto& currentFrame = callStack.back();

            // DEFENSIVE: Validate frame integrity
            if (currentFrame.localBase > constants::MAX_REASONABLE_LOCAL_BASE)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - Corrupted localBase: "
                          << currentFrame.localBase << " (exceeds " << constants::MAX_REASONABLE_LOCAL_BASE << ")\n";
                return variables;
            }

            // Check if this is a lambda invocation (has originatingLambda)
            if (currentFrame.originatingLambda)
            {
                auto stackManager = vm->getStackManager();
                if (!stackManager)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Stack manager is null for lambda\n";
                    return variables;
                }

                const auto& stack = stackManager->getStack();
                const auto& lambda = currentFrame.originatingLambda;

                // DEFENSIVE: Validate lambda state
                if (currentFrame.localBase >= stack.size())
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - localBase ("
                              << currentFrame.localBase << ") exceeds stack size (" << stack.size() << ")\n";
                    return variables;
                }

                // Lambda parameters start at localBase
                if (lambda->parameterCount > constants::MAX_REASONABLE_PARAM_COUNT)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Unreasonable lambda parameter count: "
                              << lambda->parameterCount << "\n";
                    return variables;
                }

                for (size_t i = 0; i < lambda->parameterCount; ++i)
                {
                    // DEFENSIVE: Check for overflow
                    if (currentFrame.localBase + i < currentFrame.localBase)
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in stack index calculation\n";
                        break;
                    }

                    size_t stackIndex = currentFrame.localBase + i;
                    if (stackIndex < stack.size())
                    {
                        try
                        {
                            // Use actual parameter name if available, otherwise use generic name
                            std::string paramName = (i < lambda->parameterNames.size() && !lambda->parameterNames[i].empty())
                                ? lambda->parameterNames[i]
                                : "param_" + std::to_string(i);
                            variables.push_back(valueToDebugVariable(paramName, stack[stackIndex]));
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "VMVariableInspector::getLocalVariables() - Error getting lambda parameter "
                                      << i << ": " << e.what() << "\n";
                        }
                    }
                }

                // Captured variables come after parameters
                for (size_t i = 0; i < lambda->capturedSlots.size(); ++i)
                {
                    // DEFENSIVE: Check for overflow in index calculation
                    size_t offset = lambda->parameterCount + i;
                    if (offset < lambda->parameterCount)  // Overflow check
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow calculating captured variable offset\n";
                        break;
                    }

                    if (currentFrame.localBase + offset < currentFrame.localBase)  // Overflow check
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in captured variable stack index\n";
                        break;
                    }

                    size_t stackIndex = currentFrame.localBase + offset;
                    if (stackIndex < stack.size())
                    {
                        try
                        {
                            // Use actual captured variable name if available
                            std::string capturedName = (i < lambda->capturedNames.size() && !lambda->capturedNames[i].empty())
                                ? lambda->capturedNames[i]
                                : "captured_" + std::to_string(i);
                            variables.push_back(valueToDebugVariable(capturedName, stack[stackIndex]));
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "VMVariableInspector::getLocalVariables() - Error getting captured variable "
                                      << i << ": " << e.what() << "\n";
                        }
                    }
                }

                // Also show locals declared inside the lambda
                // Get their names from the lambda's function metadata
                const auto* program = vm->getProgram();
                if (program && !lambda->functionName.empty())
                {
                    const auto* lambdaMetadata = program->getFunction(lambda->functionName);
                    if (lambdaMetadata && !lambdaMetadata->localVariableNames.empty())
                    {
                        // Show locals starting after parameters + captured variables
                        size_t localStartIndex = lambda->parameterCount + lambda->capturedSlots.size();

                        // DEFENSIVE: Check for overflow in localStartIndex
                        if (localStartIndex < lambda->parameterCount)
                        {
                            std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in localStartIndex\n";
                            return variables;
                        }

                        for (size_t i = localStartIndex; i < lambdaMetadata->localVariableNames.size(); ++i)
                        {
                            // DEFENSIVE: Check for overflow
                            if (currentFrame.localBase + i < currentFrame.localBase)
                            {
                                std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in lambda local stack index\n";
                                break;
                            }

                            size_t stackIndex = currentFrame.localBase + i;
                            if (stackIndex < stack.size())
                            {
                                try
                                {
                                    const auto& val = stack[stackIndex];
                                    // Only show initialized variables
                                    if (!std::holds_alternative<std::monostate>(val))
                                    {
                                        std::string localName = lambdaMetadata->localVariableNames[i];
                                        if (localName.empty())
                                        {
                                            localName = "local_" + std::to_string(i - localStartIndex);
                                        }
                                        variables.push_back(valueToDebugVariable(localName, val));
                                    }
                                }
                                catch (const std::exception& e)
                                {
                                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting lambda local variable "
                                              << i << ": " << e.what() << "\n";
                                }
                            }
                            else
                            {
                                break; // Don't go past stack size
                            }
                        }
                    }
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
                std::cerr << "VMVariableInspector::getLocalVariables() - Stack manager is null\n";
                return variables;
            }

            const auto& stack = stackManager->getStack();

            // DEFENSIVE: Validate frame state
            if (currentFrame.localBase >= stack.size())
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - localBase ("
                          << currentFrame.localBase << ") exceeds stack size (" << stack.size() << ")\n";
                return variables;
            }

            // First, add parameters (we have names for these)
            if (funcMetadata->parameterCount > constants::MAX_REASONABLE_PARAM_COUNT)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - Unreasonable parameter count: "
                          << funcMetadata->parameterCount << "\n";
                return variables;
            }

            for (size_t i = 0; i < funcMetadata->parameterCount && i < funcMetadata->parameterNames.size(); ++i)
            {
                // DEFENSIVE: Check for overflow
                if (currentFrame.localBase + i < currentFrame.localBase)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in parameter stack index\n";
                    break;
                }

                size_t stackIndex = currentFrame.localBase + i;
                if (stackIndex < stack.size())
                {
                    try
                    {
                        const auto& val = stack[stackIndex];
                        variables.push_back(valueToDebugVariable(funcMetadata->parameterNames[i], val));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Error getting parameter "
                                  << i << ": " << e.what() << "\n";
                    }
                }
            }

            // Add other local variables if we have names for them
            if (!funcMetadata->localVariableNames.empty())
            {
                for (size_t i = 0; i < funcMetadata->localVariableNames.size(); ++i)
                {
                    // DEFENSIVE: Check for overflow
                    if (currentFrame.localBase + i < currentFrame.localBase)
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in local variable stack index\n";
                        break;
                    }

                    size_t stackIndex = currentFrame.localBase + i;
                    if (stackIndex < stack.size())
                    {
                        try
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
                        catch (const std::exception& e)
                        {
                            std::cerr << "VMVariableInspector::getLocalVariables() - Error getting local variable "
                                      << i << ": " << e.what() << "\n";
                        }
                    }
                }
            }
            else
            {
                // For now, add unnamed locals as "local_N"
                for (size_t i = funcMetadata->parameterCount; i < funcMetadata->localCount; ++i)
                {
                    // DEFENSIVE: Check for overflow
                    if (currentFrame.localBase + i < currentFrame.localBase)
                    {
                        std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in unnamed local stack index\n";
                        break;
                    }

                    size_t stackIndex = currentFrame.localBase + i;
                    std::string varName = "local_" + std::to_string(i);

                    if (stackIndex < stack.size())
                    {
                        try
                        {
                            const auto& val = stack[stackIndex];
                            variables.push_back(valueToDebugVariable(varName, val));
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "VMVariableInspector::getLocalVariables() - Error getting unnamed local variable "
                                      << i << ": " << e.what() << "\n";
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - Unknown exception\n";
        }

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getGlobalVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            // DEFENSIVE: Validate VM is not null
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getGlobalVariables() - VM is null\n";
                return variables;
            }

            // Get global variable metadata from the bytecode program
            const auto* program = vm->getProgram();
            if (!program)
            {
                // Not an error - program might not have globals
                return variables;
            }

            const auto& globalMetadata = program->getGlobalVariables();
            if (globalMetadata.empty())
            {
                // Not an error - no globals defined
                return variables;
            }

            // Get the environment to access actual global variable values
            auto env = vm->getEnvironment();
            if (!env)
            {
                std::cerr << "VMVariableInspector::getGlobalVariables() - Environment is null\n";
                return variables;
            }

            // For each global variable in metadata, try to get its value
            for (const auto& meta : globalMetadata)
            {
                try
                {
                    // Try to get the variable definition from the environment
                    auto varDef = env->findVariable(meta.name);
                    if (varDef)
                    {
                        try
                        {
                            // DEFENSIVE: getValue() might throw if variable is in invalid state
                            value::Value val = varDef->getValue();
                            variables.push_back(valueToDebugVariable(meta.name, val));
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "VMVariableInspector::getGlobalVariables() - Error getting value for '"
                                      << meta.name << "': " << e.what() << "\n";
                            // Continue with other variables
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getGlobalVariables() - Error accessing global variable '"
                              << meta.name << "': " << e.what() << "\n";
                    // Continue with other variables
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getGlobalVariables() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getGlobalVariables() - Unknown exception\n";
        }

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getStaticVariables(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            // DEFENSIVE: Validate VM is not null
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - VM is null\n";
                return variables;
            }

            // Get the environment to access the class registry
            auto env = vm->getEnvironment();
            if (!env)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - Environment is null\n";
                return variables;
            }

            // Get the class registry
            auto classRegistry = env->getClassRegistry();
            if (!classRegistry)
            {
                // Not an error - registry might not be initialized yet
                return variables;
            }

            // Get all registered class names
            std::vector<std::string> classNames;
            try
            {
                classNames = classRegistry->getAllItemNames();
            }
            catch (const std::exception& e)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - Error getting class names: "
                          << e.what() << "\n";
                return variables;
            }

            // Iterate through all classes and their static fields
            for (const auto& className : classNames)
            {
                try
                {
                    auto classDef = classRegistry->findClass(className);
                    if (!classDef)
                    {
                        continue;
                    }

                    // DEFENSIVE: getStaticFields() might throw
                    try
                    {
                        const auto& staticFields = classDef->getStaticFields();
                        for (const auto& [fieldName, fieldDef] : staticFields)
                        {
                            if (!fieldDef)
                            {
                                continue;
                            }

                            try
                            {
                                // Format as ClassName::fieldName
                                std::string qualifiedName = className + "::" + fieldName;

                                // DEFENSIVE: getValue() might throw if field is in invalid state
                                value::Value val = fieldDef->getValue();
                                variables.push_back(valueToDebugVariable(qualifiedName, val));
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "VMVariableInspector::getStaticVariables() - Error getting static field '"
                                          << className << "::" << fieldName << "': " << e.what() << "\n";
                                // Continue with other fields
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getStaticVariables() - Error accessing static fields for class '"
                                  << className << "': " << e.what() << "\n";
                        // Continue with other classes
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getStaticVariables() - Error processing class '"
                              << className << "': " << e.what() << "\n";
                    // Continue with other classes
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getStaticVariables() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getStaticVariables() - Unknown exception\n";
        }

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getVariableChildren(
        std::shared_ptr<vm::runtime::VirtualMachine> vm, int64_t refId)
    {
        std::vector<DebugVariable> children;

        try
        {
            // DEFENSIVE: Validate refId exists in cache
            auto it = refIdToValue.find(refId);
            if (it == refIdToValue.end())
            {
                // Not an error - just means refId not found or expired
                return children;
            }

            const auto& val = it->second;

            // Handle arrays
            if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val))
            {
                auto arr = std::get<std::shared_ptr<value::NativeArray>>(val);
                if (arr)
                {
                    try
                    {
                        // DEFENSIVE: size() might throw if array is corrupted
                        size_t arrSize = arr->size();
                        size_t limit = std::min(arrSize, constants::MAX_ARRAY_DISPLAY_ELEMENTS);

                        for (size_t i = 0; i < limit; ++i)
                        {
                            try
                            {
                                std::string indexName = "[" + std::to_string(i) + "]";
                                // DEFENSIVE: get() might throw if index is invalid or array state is corrupted
                                value::Value element = arr->get(i);
                                children.push_back(valueToDebugVariable(indexName, element));
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "VMVariableInspector::getVariableChildren() - Error getting array element "
                                          << i << ": " << e.what() << "\n";
                                // Continue with other elements
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getVariableChildren() - Error accessing array: "
                                  << e.what() << "\n";
                    }
                }
            }
            // Handle objects
            else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
                if (obj)
                {
                    try
                    {
                        // DEFENSIVE: getAllFieldValues() might throw if object is corrupted
                        const auto& fields = obj->getAllFieldValues();
                        for (const auto& [fieldName, fieldValue] : fields)
                        {
                            try
                            {
                                children.push_back(valueToDebugVariable(fieldName, fieldValue));
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "VMVariableInspector::getVariableChildren() - Error getting object field '"
                                          << fieldName << "': " << e.what() << "\n";
                                // Continue with other fields
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getVariableChildren() - Error accessing object fields: "
                                  << e.what() << "\n";
                    }
                }
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

    DebugVariable VMVariableInspector::valueToDebugVariable(const std::string& name, const value::Value& val)
    {
        // Check if this is an expandable type
        bool expandable = std::holds_alternative<std::shared_ptr<value::NativeArray>>(val) ||
                         std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

        int64_t refId = 0;
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
        try
        {
            if (std::holds_alternative<std::monostate>(val))
            {
                return "null";
            }
            else if (std::holds_alternative<int64_t>(val))
            {
                return std::to_string(std::get<int64_t>(val));
            }
            else if (std::holds_alternative<double>(val))
            {
                return std::to_string(std::get<double>(val));
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
                try
                {
                    auto internedStr = std::get<value::InternedString>(val);
                    return "\"" + internedStr.getString() + "\"";
                }
                catch (const std::exception& e)
                {
                    return "\"<error: " + std::string(e.what()) + ">\"";
                }
            }
            else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val))
            {
                auto arr = std::get<std::shared_ptr<value::NativeArray>>(val);
                if (arr)
                {
                    try
                    {
                        // DEFENSIVE: size() might throw if array is corrupted
                        return "Array[" + std::to_string(arr->size()) + "]";
                    }
                    catch (const std::exception& e)
                    {
                        return "Array[error: " + std::string(e.what()) + "]";
                    }
                }
                return "Array[null]";
            }
            else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
                if (obj)
                {
                    try
                    {
                        // DEFENSIVE: getTypeName() might throw
                        return obj->getTypeName() + " instance";
                    }
                    catch (const std::exception& e)
                    {
                        return "Object[error: " + std::string(e.what()) + "]";
                    }
                }
                return "Object[null]";
            }
            else if (std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(val))
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
            if (std::holds_alternative<std::monostate>(val))
            {
                return "null";
            }
            else if (std::holds_alternative<int64_t>(val))
            {
                return "Int";
            }
            else if (std::holds_alternative<double>(val))
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
                    try
                    {
                        // DEFENSIVE: getTypeName() might throw
                        return obj->getTypeName();
                    }
                    catch (const std::exception&)
                    {
                        return "Object[error]";
                    }
                }
                return "Object";
            }
            else if (std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(val))
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
        catch (const std::exception&)
        {
            return "Error";
        }
        catch (...)
        {
            return "Unknown";
        }
    }
}
