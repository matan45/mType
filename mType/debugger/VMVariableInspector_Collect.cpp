/**
 * VMVariableInspector — local-variable enumeration.
 *
 * Holds getLocalVariables and its per-frame-kind helpers
 * (collectTopLevelScriptLocals / collectLambdaFrameLocals /
 * collectFunctionFrameLocals). Per-loop work is factored into
 * anonymous-namespace templates that accept a `toVar` callable, so the
 * member methods stay under the 50-line cap without needing private
 * access to valueToDebugVariable.
 *
 * Sibling translation units:
 *   - VMVariableInspector_Find.cpp    — findVariableValue + per-kind lookup helpers
 *   - VMVariableInspector_Globals.cpp — getGlobalVariables / getStaticVariables
 *   - VMVariableInspector_Format.cpp  — getVariableChildren / valueToString / getTypeName
 */

#include "VMVariableInspector.hpp"
#include <cstddef>
#include <iostream>
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
        // True if adding `i` to `localBase` would wrap. Mirrors the
        // size_t-overflow guards in the original code.
        bool stackIndexOverflows(size_t localBase, size_t i)
        {
            return localBase + i < localBase;
        }

        template <typename ToVar>
        void collectLambdaParamsImpl(const std::vector<value::Value>& stack,
                                     const vm::runtime::CallFrame& frame,
                                     const vm::runtime::BytecodeLambda& lambda,
                                     ToVar&& toVar,
                                     std::vector<DebugVariable>& variables)
        {
            if (lambda.parameterCount > constants::MAX_REASONABLE_PARAM_COUNT)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - Unreasonable lambda parameter count: "
                          << lambda.parameterCount << "\n";
                return;
            }

            for (size_t i = 0; i < lambda.parameterCount; ++i)
            {
                if (stackIndexOverflows(frame.localBase, i))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in stack index calculation\n";
                    break;
                }

                size_t stackIndex = frame.localBase + i;
                if (stackIndex >= stack.size())
                {
                    continue;
                }
                try
                {
                    std::string paramName = (i < lambda.parameterNames.size() && !lambda.parameterNames[i].empty())
                        ? lambda.parameterNames[i]
                        : "param_" + std::to_string(i);
                    variables.push_back(toVar(paramName, stack[stackIndex]));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting lambda parameter "
                              << i << ": " << e.what() << "\n";
                }
            }
        }

        template <typename ToVar>
        void collectLambdaCapturesImpl(const std::vector<value::Value>& stack,
                                       const vm::runtime::CallFrame& frame,
                                       const vm::runtime::BytecodeLambda& lambda,
                                       ToVar&& toVar,
                                       std::vector<DebugVariable>& variables)
        {
            for (size_t i = 0; i < lambda.capturedSlots.size(); ++i)
            {
                size_t offset = lambda.parameterCount + i;
                if (offset < lambda.parameterCount)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow calculating captured variable offset\n";
                    break;
                }
                if (stackIndexOverflows(frame.localBase, offset))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in captured variable stack index\n";
                    break;
                }

                size_t stackIndex = frame.localBase + offset;
                if (stackIndex >= stack.size())
                {
                    continue;
                }
                try
                {
                    std::string capturedName = (i < lambda.capturedNames.size() && !lambda.capturedNames[i].empty())
                        ? lambda.capturedNames[i]
                        : "captured_" + std::to_string(i);
                    variables.push_back(toVar(capturedName, stack[stackIndex]));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting captured variable "
                              << i << ": " << e.what() << "\n";
                }
            }
        }

        template <typename ToVar>
        void collectLambdaBodyLocalsImpl(const std::vector<value::Value>& stack,
                                         const vm::runtime::CallFrame& frame,
                                         const vm::runtime::BytecodeLambda& lambda,
                                         const vm::bytecode::BytecodeProgram& program,
                                         ToVar&& toVar,
                                         std::vector<DebugVariable>& variables)
        {
            if (lambda.functionName == vm::bytecode::INVALID_FN_HANDLE)
            {
                return;
            }
            // MYT-197: O(1) handle-keyed metadata lookup.
            const auto* lambdaMeta = program.getFunctionMeta(lambda.functionName);
            if (!lambdaMeta || lambdaMeta->localVariableNames.empty())
            {
                return;
            }

            size_t localStartIndex = lambda.parameterCount + lambda.capturedSlots.size();
            if (localStartIndex < lambda.parameterCount)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in localStartIndex\n";
                return;
            }

            for (size_t i = localStartIndex; i < lambdaMeta->localVariableNames.size(); ++i)
            {
                if (stackIndexOverflows(frame.localBase, i))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in lambda local stack index\n";
                    break;
                }
                size_t stackIndex = frame.localBase + i;
                if (stackIndex >= stack.size())
                {
                    break; // Don't go past stack size.
                }
                try
                {
                    const auto& val = stack[stackIndex];
                    if (value::isVoid(val))
                    {
                        continue; // Only show initialized variables.
                    }
                    std::string localName = lambdaMeta->localVariableNames[i];
                    if (localName.empty())
                    {
                        localName = "local_" + std::to_string(i - localStartIndex);
                    }
                    variables.push_back(toVar(localName, val));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting lambda local variable "
                              << i << ": " << e.what() << "\n";
                }
            }
        }

        template <typename ToVar>
        void collectFunctionParamsImpl(const std::vector<value::Value>& stack,
                                       const vm::runtime::CallFrame& frame,
                                       const vm::bytecode::BytecodeProgram::FunctionMetadata& meta,
                                       ToVar&& toVar,
                                       std::vector<DebugVariable>& variables)
        {
            for (size_t i = 0; i < meta.parameterCount && i < meta.parameterNames.size(); ++i)
            {
                if (stackIndexOverflows(frame.localBase, i))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in parameter stack index\n";
                    break;
                }
                size_t stackIndex = frame.localBase + i;
                if (stackIndex >= stack.size())
                {
                    continue;
                }
                try
                {
                    variables.push_back(toVar(meta.parameterNames[i], stack[stackIndex]));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting parameter "
                              << i << ": " << e.what() << "\n";
                }
            }
        }

        template <typename ToVar>
        void collectFunctionNamedLocalsImpl(const std::vector<value::Value>& stack,
                                            const vm::runtime::CallFrame& frame,
                                            const vm::bytecode::BytecodeProgram::FunctionMetadata& meta,
                                            ToVar&& toVar,
                                            std::vector<DebugVariable>& variables)
        {
            for (size_t i = 0; i < meta.localVariableNames.size(); ++i)
            {
                if (stackIndexOverflows(frame.localBase, i))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in local variable stack index\n";
                    break;
                }
                size_t stackIndex = frame.localBase + i;
                if (stackIndex >= stack.size())
                {
                    continue;
                }
                // Skip parameters (already added) and empty/uninitialized slots.
                if (meta.localVariableNames[i].empty() || i < meta.parameterCount)
                {
                    continue;
                }
                try
                {
                    const auto& val = stack[stackIndex];
                    // localVariableNames captures all locals at compile time; only
                    // show ones that have been initialized at this PC.
                    if (!value::isVoid(val))
                    {
                        variables.push_back(toVar(meta.localVariableNames[i], val));
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting local variable "
                              << i << ": " << e.what() << "\n";
                }
            }
        }

        template <typename ToVar>
        void collectFunctionUnnamedLocalsImpl(const std::vector<value::Value>& stack,
                                              const vm::runtime::CallFrame& frame,
                                              const vm::bytecode::BytecodeProgram::FunctionMetadata& meta,
                                              ToVar&& toVar,
                                              std::vector<DebugVariable>& variables)
        {
            for (size_t i = meta.parameterCount; i < meta.localCount; ++i)
            {
                if (stackIndexOverflows(frame.localBase, i))
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Integer overflow in unnamed local stack index\n";
                    break;
                }
                size_t stackIndex = frame.localBase + i;
                if (stackIndex >= stack.size())
                {
                    continue;
                }
                try
                {
                    variables.push_back(toVar("local_" + std::to_string(i), stack[stackIndex]));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getLocalVariables() - Error getting unnamed local variable "
                              << i << ": " << e.what() << "\n";
                }
            }
        }
    } // namespace

    void VMVariableInspector::collectTopLevelScriptLocals(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        std::vector<DebugVariable>& variables)
    {
        auto stackManager = vm->getStackManager();
        const auto* program = vm->getProgram();
        if (!stackManager || !program)
        {
            return;
        }

        const auto& callStack = vm->getCallStack();
        auto topLevelSlots = collectTopLevelLocalSlots(vm);
        const auto& stack = stackManager->getStack();
        for (const auto& [name, slot] : topLevelSlots)
        {
            const size_t stackIndex = callStack.empty()
                ? slot
                : callStack.back().localBase + slot;
            if (stackIndex < stack.size() && !value::isVoid(stack[stackIndex]))
            {
                variables.push_back(valueToDebugVariable(name, stack[stackIndex]));
            }
        }
    }

    void VMVariableInspector::collectLambdaFrameLocals(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        const vm::runtime::CallFrame& frame,
        std::vector<DebugVariable>& variables)
    {
        auto stackManager = vm->getStackManager();
        if (!stackManager)
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - Stack manager is null for lambda\n";
            return;
        }
        const auto& stack = stackManager->getStack();
        const auto& lambda = frame.originatingLambda;

        if (frame.localBase >= stack.size())
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - localBase ("
                      << frame.localBase << ") exceeds stack size (" << stack.size() << ")\n";
            return;
        }

        auto toVar = [this](const std::string& name, const value::Value& val) {
            return valueToDebugVariable(name, val);
        };

        collectLambdaParamsImpl(stack, frame, *lambda, toVar, variables);
        collectLambdaCapturesImpl(stack, frame, *lambda, toVar, variables);

        const auto* program = vm->getProgram();
        if (program)
        {
            collectLambdaBodyLocalsImpl(stack, frame, *lambda, *program, toVar, variables);
        }
    }

    void VMVariableInspector::collectFunctionFrameLocals(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        const vm::runtime::CallFrame& frame,
        std::vector<DebugVariable>& variables)
    {
        const auto* program = vm->getProgram();
        if (!program)
        {
            return;
        }
        // MYT-197: O(1) handle-keyed metadata lookup.
        const auto* funcMeta = program->getFunctionMeta(frame.functionName);
        if (!funcMeta)
        {
            return;
        }

        auto stackManager = vm->getStackManager();
        if (!stackManager)
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - Stack manager is null\n";
            return;
        }
        const auto& stack = stackManager->getStack();
        if (frame.localBase >= stack.size())
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - localBase ("
                      << frame.localBase << ") exceeds stack size (" << stack.size() << ")\n";
            return;
        }
        if (funcMeta->parameterCount > constants::MAX_REASONABLE_PARAM_COUNT)
        {
            std::cerr << "VMVariableInspector::getLocalVariables() - Unreasonable parameter count: "
                      << funcMeta->parameterCount << "\n";
            return;
        }

        auto toVar = [this](const std::string& name, const value::Value& val) {
            return valueToDebugVariable(name, val);
        };

        collectFunctionParamsImpl(stack, frame, *funcMeta, toVar, variables);
        if (!funcMeta->localVariableNames.empty())
        {
            collectFunctionNamedLocalsImpl(stack, frame, *funcMeta, toVar, variables);
        }
        else
        {
            collectFunctionUnnamedLocalsImpl(stack, frame, *funcMeta, toVar, variables);
        }
    }

    std::vector<DebugVariable> VMVariableInspector::getLocalVariables(
        std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - VM is null\n";
                return variables;
            }

            const auto& callStack = vm->getCallStack();
            if (callStack.empty() || isTopLevelScriptFrame(vm, callStack.back()))
            {
                collectTopLevelScriptLocals(vm, variables);
                return variables;
            }

            const auto& currentFrame = callStack.back();
            if (currentFrame.localBase > constants::MAX_REASONABLE_LOCAL_BASE)
            {
                std::cerr << "VMVariableInspector::getLocalVariables() - Corrupted localBase: "
                          << currentFrame.localBase << " (exceeds " << constants::MAX_REASONABLE_LOCAL_BASE << ")\n";
                return variables;
            }

            if (currentFrame.originatingLambda)
            {
                collectLambdaFrameLocals(vm, currentFrame, variables);
            }
            else
            {
                collectFunctionFrameLocals(vm, currentFrame, variables);
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

} // namespace debugger
