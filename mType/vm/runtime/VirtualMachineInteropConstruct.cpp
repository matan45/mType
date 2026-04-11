#include "VirtualMachine.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"

namespace vm::runtime
{
    // Object construction & lambda invocation
    value::Value VirtualMachine::createObject(const std::string& className, const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException(
                "No program loaded - cannot create object in bytecode mode without compiled bytecode");
        }

        // Find class definition
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        // Create object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        // Find constructor using type-aware lookup
        auto constructor = classDef->findConstructorByTypes(args);
        if (!constructor)
        {
            // Check for default constructor case (no params, no explicit constructor)
            bool hasAnyConstructor = !classDef->getConstructors().empty();
            if (args.empty() && !hasAnyConstructor)
            {
                // Default constructor - just return the instance
                return value::Value(instance);
            }
            throw errors::RuntimeException("Constructor not found for class '" + className +
                "' with " + std::to_string(args.size()) + " parameters");
        }

        // Look for constructor bytecode using type signature (matches compiler naming)
        std::string typeSignature = constructor->getTypeSignature();
        std::string qualifiedName = typeSignature.empty()
            ? className + "::<init>"
            : className + "::<init>/" + typeSignature;
        auto* ctorMetadata = program->getFunction(qualifiedName);
        if (!ctorMetadata)
        {
            throw errors::RuntimeException("Constructor '" + qualifiedName +
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
            // Set return address to end of program so interpretLoop exits after constructor returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = qualifiedName;
            frame.thisInstance = instance;
            frame.definingClassName = className;

            pushCallFrame(frame);
            stats.functionCalls++;

            // Execute constructor
            // Use direct execution loop instead of interpretLoop() to avoid
            // recreating executors (which would invalidate the outer ExecutionContext)
            instructionPointer = ctorMetadata->startOffset;
            if (controlFlowExecutor)
            {
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= program->getInstructionCount())
                        break;
                    const auto& instr = program->getInstruction(instructionPointer);
                    executeInstruction(instr);
                    instructionPointer++;
                }
                // Clean up stack to original position
                while (stackManager->size() > frameBase)
                    stackManager->pop();
            }
            else
            {
                interpretLoop();
            }

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;

            return instance;
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

    value::Value VirtualMachine::invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                               const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException(
                "No program loaded - cannot invoke lambda in bytecode mode without compiled bytecode");
        }

        if (!lambda)
        {
            throw errors::NullPointerException("Cannot invoke null lambda");
        }

        size_t paramCount = lambda->parameterCount;
        if (args.size() != paramCount)
        {
            throw errors::RuntimeException("Lambda expects " + std::to_string(paramCount) +
                " arguments but got " + std::to_string(args.size()));
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;

        try
        {
            currentFinallyOffset = SIZE_MAX;
            size_t frameBase = stackManager->size();

            // Push arguments onto stack
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = lambda->functionName.empty() ?
                (lambda->creatingClassName.empty() ? "<lambda>" : lambda->creatingClassName + "::<lambda>") :
                lambda->functionName;
            frame.thisInstance = lambda->capturedThis;
            frame.definingClassName = lambda->creatingClassName;
            frame.originatingLambda = lambda;

            pushCallFrame(frame);
            stats.functionCalls++;

            // Create shared frame for this invocation, link to parent captures
            auto newSharedFrame = std::make_shared<SharedStackFrame>();
            newSharedFrame->parentFrame = lambda->capturedFrame;
            if (!callStack.empty())
            {
                callStack.back().sharedFrame = newSharedFrame;
            }

            // Register parameter names in shared frame
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i < lambda->parameterNames.size() && !lambda->parameterNames[i].empty())
                {
                    newSharedFrame->setLocal(lambda->parameterNames[i], i, args[i]);
                }
            }

            // Push captured variables onto stack
            if (lambda->capturedFrame)
            {
                for (size_t i = 0; i < lambda->capturedSlots.size(); ++i)
                {
                    size_t slot = lambda->capturedSlots[i];
                    value::Value capturedValue = lambda->capturedFrame->getLocal(slot);
                    push(capturedValue);
                }
            }

            // Reserve additional local variable slots if needed
            auto* lambdaMetadata = program->getFunction(lambda->functionName);
            if (lambdaMetadata)
            {
                size_t pushedSlots = args.size() + lambda->capturedSlots.size();
                if (lambdaMetadata->localCount > pushedSlots)
                {
                    size_t additionalLocals = lambdaMetadata->localCount - pushedSlots;
                    for (size_t i = 0; i < additionalLocals; ++i)
                    {
                        push(std::monostate{});
                    }
                }
            }

            // Execute lambda
            instructionPointer = lambda->instructionPointer;
            value::Value result = std::monostate{};
            if (controlFlowExecutor)
            {
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= program->getInstructionCount())
                        break;
                    const auto& instr = program->getInstruction(instructionPointer);
                    executeInstruction(instr);
                    instructionPointer++;
                }
                if (stackManager->size() > frameBase)
                    result = stackManager->pop();
                while (stackManager->size() > frameBase)
                    stackManager->pop();
            }
            else
            {
                result = interpretLoop();
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
