#include "VirtualMachine.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"

namespace vm::runtime
{
    value::Value VirtualMachine::invokeMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                              const std::string& methodName,
                                              const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException(
                "No program loaded - cannot invoke method in bytecode mode without compiled bytecode");
        }

        if (!instance)
        {
            throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object");
        }

        auto classDef = instance->getClassDefinition();
        size_t argCount = args.size();

        // Find method in hierarchy
        auto method = classDef->findInstanceMethodInHierarchy(methodName, argCount);
        if (!method)
        {
            throw errors::RuntimeException("Instance method not found: " + methodName +
                " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        // Find which class defines this method and get the type signature
        std::string definingClassName = classDef->getName();
        std::string typeSignature;
        auto currentClass = classDef;
        while (currentClass)
        {
            auto localMethod = currentClass->findInstanceMethod(methodName, argCount);
            if (localMethod)
            {
                definingClassName = currentClass->getName();
                typeSignature = localMethod->getTypeSignature();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        // Look for method bytecode using type signature (matches compiler naming)
        std::string qualifiedName = typeSignature.empty()
            ? definingClassName + "::" + methodName
            : definingClassName + "::" + methodName + "/" + typeSignature;
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Method '" + qualifiedName +
                "' has no bytecode. Bytecode compilation is required for VM execution.");
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;

        try
        {
            // Reset finally offset for new function call
            currentFinallyOffset = SIZE_MAX;

            // Push instance and arguments onto stack
            size_t frameBase = stackManager->size();
            push(instance);
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            // Set return address to end of program so interpretLoop exits after method returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = qualifiedName;
            frame.thisInstance = instance;
            frame.definingClassName = definingClassName;

            pushCallFrame(frame);
            stats.functionCalls++;

            // Execute method (direct loop to avoid recreating executors)
            instructionPointer = funcMetadata->startOffset;
            value::Value result = std::monostate{};
            if (controlFlowExecutor)
            {
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= program->getInstructionCount())
                        break;
                    if (suspendedByAwait)
                        break;
                    const auto& instr = program->getInstruction(instructionPointer);
                    executeInstruction(instr);
                    instructionPointer++;
                }
                if (!suspendedByAwait && stackManager->size() > frameBase)
                    result = stackManager->pop();
                if (!suspendedByAwait)
                {
                    while (stackManager->size() > frameBase)
                        stackManager->pop();
                }
            }
            else
            {
                result = interpretLoop();
            }

            // If suspended by await, don't restore state — EventLoop will resume
            if (suspendedByAwait)
            {
                suspendedByAwait = false;
                return result;
            }

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;

            return result;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
    }

    value::Value VirtualMachine::invokeStaticMethod(const std::string& className,
                                                    const std::string& methodName,
                                                    const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException(
                "No program loaded - cannot invoke static method in bytecode mode without compiled bytecode");
        }

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        size_t argCount = args.size();
        auto method = classDef->findStaticMethod(methodName, argCount);
        if (!method)
        {
            throw errors::RuntimeException("Static method not found: " + methodName +
                " with " + std::to_string(argCount) + " arguments in class " + className);
        }

        // Look for method bytecode using type signature (matches compiler naming)
        std::string typeSignature = method->getTypeSignature();
        std::string qualifiedName = typeSignature.empty()
            ? className + "::" + methodName
            : className + "::" + methodName + "/" + typeSignature;
        // For static methods, append $static suffix
        if (method->isStatic()) {
            qualifiedName += "$static";
        }
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Static method '" + qualifiedName +
                "' has no bytecode. Bytecode compilation is required for VM execution.");
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;

        try
        {
            // Reset finally offset for new function call
            currentFinallyOffset = SIZE_MAX;

            // Push arguments onto stack
            size_t frameBase = stackManager->size();
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            // Set return address to end of program so interpretLoop exits after method returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = qualifiedName;
            frame.thisInstance = nullptr; // Static methods have no 'this'
            frame.definingClassName = className;

            pushCallFrame(frame);
            stats.functionCalls++;

            // Execute method (direct loop to avoid recreating executors)
            instructionPointer = funcMetadata->startOffset;
            value::Value result = std::monostate{};
            if (controlFlowExecutor)
            {
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= program->getInstructionCount())
                        break;
                    if (suspendedByAwait)
                        break;
                    const auto& instr = program->getInstruction(instructionPointer);
                    executeInstruction(instr);
                    instructionPointer++;
                }
                if (!suspendedByAwait && stackManager->size() > frameBase)
                    result = stackManager->pop();
                if (!suspendedByAwait)
                {
                    while (stackManager->size() > frameBase)
                        stackManager->pop();
                }
            }
            else
            {
                result = interpretLoop();
            }

            // If suspended by await, don't restore state — EventLoop will resume
            if (suspendedByAwait)
            {
                suspendedByAwait = false;
                return result;
            }

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;

            return result;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
    }
}
