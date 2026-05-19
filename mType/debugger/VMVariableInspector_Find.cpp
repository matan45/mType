/**
 * VMVariableInspector — findVariableValue and its per-storage-kind helpers.
 *
 * Resolves a debugger-visible name to a VM value by searching, in order:
 *   1. ClassName::staticField (qualified static field)
 *   2. Top-level script locals when the call stack is empty or in the
 *      synthetic __script_main__ frame
 *   3. The active function frame: "this", lambda params/captures/locals,
 *      or regular function params/locals
 *   4. Environment globals as a last fallback
 *
 * The orchestrator and per-storage helpers all live here so the search
 * order is obvious from a single file. ctor / clearCache / formatValue
 * are in VMVariableInspector.cpp; getLocalVariables and friends are in
 * VMVariableInspector_Collect.cpp.
 */

#include "VMVariableInspector.hpp"
#include <cstddef>
#include "DebuggerConstants.hpp"
#include "../vm/runtime/stack/StackManager.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../value/ValueShim.hpp"
#include "../environment/manager/VariableManager.hpp"

namespace debugger
{
    namespace
    {
        std::optional<value::Value> findStaticFieldValue(
            std::shared_ptr<vm::runtime::VirtualMachine> vm,
            const std::string& className,
            const std::string& fieldName)
        {
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

        std::optional<value::Value> findThisInFrame(const vm::runtime::CallFrame& frame)
        {
            if (frame.thisInstance)
            {
                return value::Value(frame.thisInstance);
            }
            if (frame.thisInstanceRaw)
            {
                return value::makeStackObjectValue(frame.thisInstanceRaw);
            }
            if (frame.originatingLambda && frame.originatingLambda->capturedThis)
            {
                return value::Value(frame.originatingLambda->capturedThis);
            }
            return std::nullopt;
        }

        std::optional<value::Value> findInLambdaFrame(
            const std::string& name,
            const vm::runtime::CallFrame& frame,
            const std::vector<value::Value>& stack,
            const vm::bytecode::BytecodeProgram* program)
        {
            const auto& lambda = frame.originatingLambda;
            for (size_t i = 0; i < lambda->parameterNames.size(); ++i)
            {
                if (lambda->parameterNames[i] == name)
                {
                    size_t stackIndex = frame.localBase + i;
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
                    size_t stackIndex = frame.localBase + lambda->parameterCount + i;
                    if (stackIndex < stack.size())
                    {
                        return stack[stackIndex];
                    }
                }
            }

            const auto* lambdaMeta = program->getFunctionMeta(lambda->functionName);
            if (!lambdaMeta)
            {
                return std::nullopt;
            }
            for (size_t i = 0; i < lambdaMeta->localVariableNames.size(); ++i)
            {
                if (lambdaMeta->localVariableNames[i] == name)
                {
                    size_t stackIndex = frame.localBase + i;
                    if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
                    {
                        return stack[stackIndex];
                    }
                }
            }
            return std::nullopt;
        }

        std::optional<value::Value> findInFunctionFrame(
            const std::string& name,
            const vm::runtime::CallFrame& frame,
            const std::vector<value::Value>& stack,
            const vm::bytecode::BytecodeProgram* program)
        {
            const auto* funcMeta = program->getFunctionMeta(frame.functionName);
            if (!funcMeta)
            {
                return std::nullopt;
            }

            for (size_t i = 0; i < funcMeta->parameterNames.size(); ++i)
            {
                if (funcMeta->parameterNames[i] == name)
                {
                    size_t stackIndex = frame.localBase + i;
                    if (stackIndex < stack.size())
                    {
                        return stack[stackIndex];
                    }
                }
            }

            for (size_t i = 0; i < funcMeta->localVariableNames.size(); ++i)
            {
                if (funcMeta->localVariableNames[i] == name)
                {
                    size_t stackIndex = frame.localBase + i;
                    if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
                    {
                        return stack[stackIndex];
                    }
                }
            }
            return std::nullopt;
        }

        std::optional<value::Value> findInGlobals(
            std::shared_ptr<vm::runtime::VirtualMachine> vm,
            const std::string& name)
        {
            auto env = vm->getEnvironment();
            if (!env)
            {
                return std::nullopt;
            }
            auto varDef = env->findVariable(name);
            if (!varDef)
            {
                return std::nullopt;
            }
            return varDef->getValue();
        }
    } // namespace

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
            return findStaticFieldValue(vm, name.substr(0, staticSep), name.substr(staticSep + 2));
        }

        const auto& callStack = vm->getCallStack();
        if (callStack.empty() || isTopLevelScriptFrame(vm, callStack.back()))
        {
            auto stackManager = vm->getStackManager();
            if (stackManager)
            {
                auto topLevelSlots = collectTopLevelLocalSlots(vm);
                auto slotIt = topLevelSlots.find(name);
                if (slotIt != topLevelSlots.end())
                {
                    const auto& stack = stackManager->getStack();
                    const size_t stackIndex = callStack.empty()
                        ? slotIt->second
                        : callStack.back().localBase + slotIt->second;
                    if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
                    {
                        return stack[stackIndex];
                    }
                }
            }
        }

        if (!callStack.empty())
        {
            const auto& currentFrame = callStack.back();

            if (name == "this")
            {
                if (auto thisVal = findThisInFrame(currentFrame))
                {
                    return thisVal;
                }
            }

            auto stackManager = vm->getStackManager();
            const auto* program = vm->getProgram();
            if (stackManager && program && currentFrame.localBase <= constants::MAX_REASONABLE_LOCAL_BASE)
            {
                const auto& stack = stackManager->getStack();
                if (currentFrame.localBase < stack.size())
                {
                    auto framed = currentFrame.originatingLambda
                        ? findInLambdaFrame(name, currentFrame, stack, program)
                        : findInFunctionFrame(name, currentFrame, stack, program);
                    if (framed)
                    {
                        return framed;
                    }
                }
            }
        }

        return findInGlobals(vm, name);
    }
} // namespace debugger
