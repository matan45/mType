#include "VMVariableInspector.hpp"
#include <cstddef>
#include "DebuggerConstants.hpp"
#include "../vm/runtime/stack/StackManager.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"
#include "../environment/manager/VariableManager.hpp"
#include <iostream>
#include <sstream>

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

    std::optional<value::Value> VMVariableInspector::findVariableValue(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        const std::string& name)
    {
        if (!vm || name.empty())
        {
            return std::nullopt;
        }

        const size_t staticSep = name.find("::");
        if (staticSep != std::string::npos)
        {
            const std::string className = name.substr(0, staticSep);
            const std::string fieldName = name.substr(staticSep + 2);
            auto env = vm->getEnvironment();
            if (!env)
            {
                return std::nullopt;
            }
            auto classRegistry = env->getClassRegistry();
            if (!classRegistry)
            {
                return std::nullopt;
            }
            auto classDef = classRegistry->findClass(className);
            if (!classDef)
            {
                return std::nullopt;
            }
            const auto& staticFields = classDef->getStaticFields();
            auto it = staticFields.find(fieldName);
            if (it == staticFields.end() || !it->second)
            {
                return std::nullopt;
            }
            return it->second->getValue();
        }

        const auto& callStack = vm->getCallStack();
        if (!callStack.empty())
        {
            const auto& currentFrame = callStack.back();

            if (name == "this")
            {
                if (currentFrame.thisInstance)
                {
                    return value::Value(currentFrame.thisInstance);
                }
                if (currentFrame.thisInstanceRaw)
                {
                    return value::makeStackObjectValue(currentFrame.thisInstanceRaw);
                }
                if (currentFrame.originatingLambda && currentFrame.originatingLambda->capturedThis)
                {
                    return value::Value(currentFrame.originatingLambda->capturedThis);
                }
            }

            auto stackManager = vm->getStackManager();
            const auto* program = vm->getProgram();
            if (stackManager && program && currentFrame.localBase <= constants::MAX_REASONABLE_LOCAL_BASE)
            {
                const auto& stack = stackManager->getStack();
                if (currentFrame.localBase < stack.size())
                {
                    if (currentFrame.originatingLambda)
                    {
                        const auto& lambda = currentFrame.originatingLambda;
                        for (size_t i = 0; i < lambda->parameterNames.size(); ++i)
                        {
                            if (lambda->parameterNames[i] == name)
                            {
                                size_t stackIndex = currentFrame.localBase + i;
                                if (stackIndex < stack.size())
                                {
                                    return stack[stackIndex];
                                }
                            }
                        }

                        for (size_t i = 0; i < lambda->capturedNames.size(); ++i)
                        {
                            if (lambda->capturedNames[i] == name)
                            {
                                size_t stackIndex = currentFrame.localBase + lambda->parameterCount + i;
                                if (stackIndex < stack.size())
                                {
                                    return stack[stackIndex];
                                }
                            }
                        }

                        const auto* lambdaMetadata = program->getFunctionMeta(lambda->functionName);
                        if (lambdaMetadata)
                        {
                            for (size_t i = 0; i < lambdaMetadata->localVariableNames.size(); ++i)
                            {
                                if (lambdaMetadata->localVariableNames[i] == name)
                                {
                                    size_t stackIndex = currentFrame.localBase + i;
                                    if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
                                    {
                                        return stack[stackIndex];
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        const auto* funcMetadata = program->getFunctionMeta(currentFrame.functionName);
                        if (funcMetadata)
                        {
                            for (size_t i = 0; i < funcMetadata->parameterNames.size(); ++i)
                            {
                                if (funcMetadata->parameterNames[i] == name)
                                {
                                    size_t stackIndex = currentFrame.localBase + i;
                                    if (stackIndex < stack.size())
                                    {
                                        return stack[stackIndex];
                                    }
                                }
                            }

                            for (size_t i = 0; i < funcMetadata->localVariableNames.size(); ++i)
                            {
                                if (funcMetadata->localVariableNames[i] == name)
                                {
                                    size_t stackIndex = currentFrame.localBase + i;
                                    if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
                                    {
                                        return stack[stackIndex];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        auto env = vm->getEnvironment();
        if (env)
        {
            auto varDef = env->findVariable(name);
            if (varDef)
            {
                return varDef->getValue();
            }
        }

        return std::nullopt;
    }

    DebugVariable VMVariableInspector::formatValue(const std::string& name, const value::Value& val)
    {
        return valueToDebugVariable(name, val);
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
                if (program && lambda->functionName != vm::bytecode::INVALID_FN_HANDLE)
                {
                    // MYT-197: O(1) handle-keyed metadata lookup.
                    const auto* lambdaMetadata = program->getFunctionMeta(lambda->functionName);
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
                                    if (!value::isVoid(val))
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

            // MYT-197: O(1) handle-keyed metadata lookup.
            const auto* funcMetadata = program->getFunctionMeta(currentFrame.functionName);
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
                                if (!value::isVoid(val))
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
            if (value::isNativeArray(val))
            {
                auto arr = value::asNativeArray(val);
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
            else if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (obj)
                {
                    try
                    {
                        // DEFENSIVE: getAllFields() might throw if object is corrupted
                        const auto& fields = obj->getAllFields();
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
        bool expandable = value::isNativeArray(val) ||
                         value::isAnyObject(val);

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
            if (value::isVoid(val))
            {
                return "null";
            }
            else if (value::isInt(val))
            {
                return std::to_string(value::asInt(val));
            }
            else if (value::isFloat(val))
            {
                return std::to_string(value::asFloat(val));
            }
            else if (value::isBool(val))
            {
                return value::asBool(val) ? "true" : "false";
            }
            else if (value::isString(val))
            {
                return "\"" + value::asString(val) + "\"";
            }
            else if (value::isInternedString(val))
            {
                try
                {
                    auto internedStr = value::asInternedString(val);
                    return "\"" + internedStr.getString() + "\"";
                }
                catch (const std::exception& e)
                {
                    return "\"<error: " + std::string(e.what()) + ">\"";
                }
            }
            else if (value::isNativeArray(val))
            {
                auto arr = value::asNativeArray(val);
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
            else if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
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
            else if (value::isLambda(val))
            {
                return "<lambda>";
            }
            else if (value::isFlatMultiArray(val))
            {
                return "<multi-array>";
            }
            else if (value::isSparseMultiArray(val))
            {
                return "<sparse-array>";
            }
            else if (value::isFlatMultiObjectArray(val))
            {
                return "<object-array>";
            }
            else if (value::isPromise(val))
            {
                return "<promise>";
            }
            else if (value::isPromiseInt(val))
            {
                return "<promise>";
            }
            else if (value::isNullType(val))
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
            if (value::isVoid(val))
            {
                return "null";
            }
            else if (value::isInt(val))
            {
                return "Int";
            }
            else if (value::isFloat(val))
            {
                return "Float";
            }
            else if (value::isBool(val))
            {
                return "Bool";
            }
            else if (value::isString(val))
            {
                return "String";
            }
            else if (value::isInternedString(val))
            {
                return "String";
            }
            else if (value::isNativeArray(val))
            {
                return "Array";
            }
            else if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
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
            else if (value::isLambda(val))
            {
                return "Lambda";
            }
            else if (value::isFlatMultiArray(val))
            {
                return "MultiArray";
            }
            else if (value::isSparseMultiArray(val))
            {
                return "SparseArray";
            }
            else if (value::isFlatMultiObjectArray(val))
            {
                return "ObjectArray";
            }
            else if (value::isPromise(val))
            {
                return "Promise";
            }
            else if (value::isPromiseInt(val))
            {
                return "Promise";
            }
            else if (value::isNullType(val))
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
