#include "VirtualMachine.hpp"
#include "executors/StackOperationsExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ArithmeticExecutor.hpp"
#include "executors/BitwiseExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/ExceptionExecutor.hpp"
#include "executors/PrimitiveMethodExecutor.hpp"  // Phase 3
#include "utils/ExceptionHandler.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/FieldNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/StringPool.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugHookHelper.hpp"
#include "../../constants/ExecutionMode.hpp"
#include "../../gc/GC.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitContext.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <iostream>

namespace vm::runtime
{
    VirtualMachine::VirtualMachine(std::shared_ptr<environment::Environment> env,
                                   size_t maxStackDepth)
        : program(nullptr)
          , stackManager(std::make_shared<StackManager>())
          , maxCallStackSize(maxStackDepth == 0 ? constants::vm::DEFAULT_MAX_CALL_STACK_SIZE : maxStackDepth)
          , instructionPointer(0)
          , environment(std::move(env))
          , eventLoop(nullptr) // Lazy initialization - only created when needed
          , currentTaskId(0)
          , suspendedByAwait(false)
          , debuggingEnabled(false)
          , currentSourceFile("")
          , currentSourceLine(0)
          , currentFinallyOffset(SIZE_MAX) // Not in finally initially
          , pendingFinallyOffset(SIZE_MAX) // No pending exception initially
          , jitEnabled(false)
    {
        callStack.reserve(constants::vm::DEFAULT_CALL_STACK_CAPACITY);

        // Initialize garbage collector if not already initialized
        if (!gc::GC::isInitialized())
        {
            gc::GC::initialize();
        }

        // Note: Executors will be initialized in execute() when program is available
        // because ExecutionContext requires a valid program pointer
    }

    VirtualMachine::~VirtualMachine() = default;

    void VirtualMachine::setJitEnabled(bool enabled)
    {
        jitEnabled = enabled;
        if (enabled && !jitProfiler)
        {
            jitProfiler = std::make_unique<jit::JitProfiler>();
            jitCodeCache = std::make_unique<jit::JitCodeCache>();
            jitCompiler = std::make_unique<jit::JitCompiler>();
        }
    }

    value::Value VirtualMachine::callFunctionFromJit(const std::string& funcName,
                                                      const std::vector<value::Value>& args)
    {
        auto funcMeta = program->getFunction(funcName);
        if (!funcMeta)
        {
            throw std::runtime_error("JIT interpreter fallback: function not found: " + funcName);
        }

        // Save current interpreter state
        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        // Push arguments as locals
        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }

        // Initialize remaining locals to null
        for (size_t i = args.size(); i < funcMeta->localCount; ++i)
        {
            stackManager->push(std::monostate{});
        }

        // Create call frame
        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        frame.functionName = funcName;
        frame.thisInstance = nullptr;
        callStack.push_back(frame);

        // Jump to function start
        instructionPointer = funcMeta->startOffset;

        // Run interpreter until this call frame is popped
        while (callStack.size() > savedCallStackDepth)
        {
            if (instructionPointer >= program->getInstructionCount())
            {
                break;
            }

            const auto& instr = program->getInstruction(instructionPointer);
            instructionPointer++;

            try
            {
                executeInstruction(instr);
            }
            catch (...)
            {
                // Restore state on exception
                while (callStack.size() > savedCallStackDepth)
                {
                    callStack.pop_back();
                }
                instructionPointer = savedIP;
                while (stackManager->size() > savedStackSize)
                {
                    stackManager->pop();
                }
                throw;
            }
        }

        // Get return value (if any)
        value::Value result = std::monostate{};
        if (stackManager->size() > savedStackSize)
        {
            result = stackManager->pop();
        }

        // Clean up stack to original size
        while (stackManager->size() > savedStackSize)
        {
            stackManager->pop();
        }

        // Restore instruction pointer
        instructionPointer = savedIP;

        return result;
    }

    ::runtime::EventLoop* VirtualMachine::ensureEventLoop()
    {
        if (!eventLoop)
        {
            eventLoop = std::make_unique<::runtime::EventLoop>();
        }
        return eventLoop.get();
    }

    value::Value VirtualMachine::execute(const bytecode::BytecodeProgram& bytecodeProgram)
    {
        program = &bytecodeProgram;

        // Check if we're resuming from a saved state
        if (savedState.has_value())
        {
            restoreState(savedState.value());
            savedState.reset(); // Clear saved state after restoring
        }
        else
        {
            // Fresh execution - start from entry point
            instructionPointer = program->getEntryPoint();
            executionStart = std::chrono::steady_clock::now();
            stats = ExecutionStats{};

            // Create an initial call frame for the implicit "main" function
            // This allows global scope variables to be captured by lambdas
            // Use "__script_main__" to match the expected name for global scope access checks
            CallFrame mainFrame;
            mainFrame.returnAddress = program->getInstructionCount(); // Return past end (halt)
            mainFrame.frameBase = 0;
            mainFrame.localBase = 0;
            mainFrame.functionName = "__script_main__";
            mainFrame.thisInstance = nullptr;
            mainFrame.definingClassName = "";
            callStack.push_back(mainFrame);
        }

        // Note: Executors are now initialized in interpretLoop() to ensure
        // they always have valid references, even when called from C++ API methods

        // Note: The main script frame is now pushed in Main.cpp before pausing at entry,
        // so VS Code has a frame to display immediately. It's also popped there after completion.

        try
        {
            value::Value result = interpretLoop();

            // Pop the main frame if it's still on the stack
            if (!callStack.empty() && callStack.back().functionName == "__script_main__") {
                callStack.pop_back();
            }

            return result;
        }
        catch (...)
        {
            // Clean up and rethrow
            reset();
            throw;
        }
    }

    value::Value VirtualMachine::executeFunction(const std::string& functionName, const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException("No program loaded");
        }

        auto* funcMetadata = program->getFunction(functionName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Function not found: " + functionName);
        }

        // Push arguments onto stack
        for (const auto& arg : args)
        {
            push(arg);
        }

        // Set instruction pointer to function start
        instructionPointer = funcMetadata->startOffset;

        return interpretLoop();
    }

    // C++ Interop API implementations
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
            instructionPointer = ctorMetadata->startOffset;
            interpretLoop();

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

            // Execute method
            instructionPointer = funcMetadata->startOffset;
            value::Value result = interpretLoop();

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

            // Execute method
            instructionPointer = funcMetadata->startOffset;
            value::Value result = interpretLoop();

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

    value::Value VirtualMachine::getField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                          const std::string& fieldName)
    {
        if (!instance)
        {
            throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object");
        }

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        return fieldDef->getValue();
    }

    void VirtualMachine::setField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                  const std::string& fieldName,
                                  const value::Value& value)
    {
        if (!instance)
        {
            throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object");
        }

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
        }

        fieldDef->setValue(value);
    }

    value::Value VirtualMachine::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef || !fieldDef->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        return fieldDef->getValue();
    }

    void VirtualMachine::setStaticField(const std::string& className,
                                        const std::string& fieldName,
                                        const value::Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef || !fieldDef->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::RuntimeException("Cannot assign to final static field '" + fieldName + "'");
        }

        fieldDef->setValue(value);
    }

    value::Value VirtualMachine::interpretLoop()
    {
        suspendedByAwait = false; // Reset flag at start

        // Initialize source location tracking by scanning forward for first instruction with location
        // This ensures error messages have correct location even before executing instructions
        if (callStack.empty() && instructionPointer == program->getEntryPoint())
        {
            for (size_t ip = instructionPointer; ip < program->getInstructionCount(); ++ip)
            {
                auto loc = program->getSourceLocation(ip);
                if (loc && !loc->filename.empty() && loc->line > 0)
                {
                    currentSourceFile = loc->filename;
                    currentSourceLine = static_cast<int>(loc->line);
                    break;
                }
            }
        }

        // Initialize executors with fresh execution context
        // This ensures executors always have valid references, even when called from C++ API
        ExecutionContext context(program, instructionPointer, callStack, maxCallStackSize,
                                 environment, stackManager, stats, executionStart,
                                 debuggingEnabled, currentSourceFile, currentSourceLine);
        stackOpsExecutor = std::make_unique<StackOperationsExecutor>(context);
        comparisonExecutor = std::make_unique<ComparisonExecutor>(context);
        logicalExecutor = std::make_unique<LogicalExecutor>(context);
        arithmeticExecutor = std::make_unique<ArithmeticExecutor>(context);
        bitwiseExecutor = std::make_unique<BitwiseExecutor>(context);
        controlFlowExecutor = std::make_unique<ControlFlowExecutor>(context);
        variableExecutor = std::make_unique<VariableExecutor>(context);
        functionExecutor = std::make_unique<FunctionExecutor>(context);
        typeExecutor = std::make_unique<TypeExecutor>(context);
        arrayExecutor = std::make_unique<ArrayExecutor>(context);
        objectExecutor = std::make_unique<ObjectExecutor>(context);
        lambdaExecutor = std::make_unique<LambdaExecutor>(context);
        exceptionExecutor = std::make_unique<ExceptionExecutor>(context);
        primitiveMethodExecutor = std::make_unique<PrimitiveMethodExecutor>(context);  // Phase 3

        // Set function executor reference in object executor for lambda-to-interface conversion
        objectExecutor->setFunctionExecutor(functionExecutor.get());

        // Initialize exception handler
        exceptionHandler = std::make_unique<utils::ExceptionHandler>(program, stackManager, callStack);

        // GC: Counter for periodic collection checks
        size_t instructionsSinceGC = 0;

        while (instructionPointer < program->getInstructionCount())
        {
            // GC: Periodic collection check
            if (++instructionsSinceGC >= gc::config::GC_CHECK_INTERVAL)
            {
                instructionsSinceGC = 0;
                gc::GC::maybeCollect();
            }

            const auto& instr = program->getInstruction(instructionPointer);

            // Check if we have a pending exception and we're hitting RETURN
            // According to Java/C# semantics: a return in finally SUPPRESSES the pending exception
            // The function returns normally and the exception is discarded
            // IMPORTANT: Only suppress if:
            // 1. We're inside a finally block (currentFinallyOffset is set)
            // 2. The IP is between FINALLY and TRY_END (we're executing finally body)
            // We can check #2 by seeing if IP > currentFinallyOffset (after FINALLY instruction)
            if (pendingException != nullptr &&
                (instr.opcode == bytecode::OpCode::RETURN || instr.opcode == bytecode::OpCode::RETURN_VALUE) &&
                currentFinallyOffset != SIZE_MAX &&
                currentFinallyOffset == pendingFinallyOffset &&
                instructionPointer > currentFinallyOffset)
            {
                // Clear the pending exception - the return inside finally overrides it
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                // Continue with normal return (do NOT re-throw)
            }

            // PERFORMANCE: Source location is tracked via LINE and SOURCE_FILE opcodes
            // No per-instruction lookup needed - saves hash map lookup on every instruction

            // Debug hook: Check for breakpoints and stepping before executing instruction
            // Only do expensive checks when debugging is actually enabled
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                if (currentSourceLine > 0)
                {
                    // Convert to errors::SourceLocation for debugger
                    errors::SourceLocation location(currentSourceFile,
                                                    currentSourceLine,
                                                    0);  // Column not tracked per-instruction

                    // Check for breakpoints/stepping
                    if (debugger::DebugHookHelper::shouldPause(location))
                    {
                        debugger::DebugHookHelper::waitForResume();
                    }
                }
            }

            try
            {
                // Execute instruction
                executeInstruction(instr);
            }
            catch (errors::UserException& e)
            {
                // Delegate exception handling to ExceptionHandler
                auto result = exceptionHandler->handleUserException(e, instructionPointer, currentFinallyOffset);

                if (!result.handled)
                {
                    // No matching catch found - check if we're in an async function
                    if (!callStack.empty())
                    {
                        const CallFrame& currentFrame = callStack.back();
                        auto funcMetadata = program->getFunction(currentFrame.functionName);

                        if (funcMetadata && funcMetadata->isAsync)
                        {
                            // We're in an async function - don't propagate exception
                            // Instead, create a rejected promise and return it

                            // Build error message from exception
                            std::string errorMsg = e.getExceptionTypeName();
                            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(e.getExceptionValue()))
                            {
                                auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(e.getExceptionValue());
                                if (objInstance)
                                {
                                    // Try to get error message from common exception fields
                                    try
                                    {
                                        value::Value msgValue = objInstance->getFieldValue("msg");
                                        if (std::holds_alternative<std::string>(msgValue))
                                        {
                                            errorMsg += ": " + std::get<std::string>(msgValue);
                                        }
                                    }
                                    catch (...)
                                    {
                                        // Field doesn't exist, try "message" field
                                        try
                                        {
                                            value::Value messageValue = objInstance->getFieldValue("message");
                                            if (std::holds_alternative<std::string>(messageValue))
                                            {
                                                errorMsg += ": " + std::get<std::string>(messageValue);
                                            }
                                        }
                                        catch (...)
                                        {
                                            // Neither field exists, use just the type name
                                        }
                                    }
                                }
                            }

                            // Create a rejected promise with the full exception object
                            auto rejectedPromise = std::make_shared<value::PromiseValue>();
                            rejectedPromise->rejectWithException(e.getExceptionValue(), e.getExceptionTypeName(), errorMsg);

                            // Clean up call frame and return the rejected promise
                            controlFlowExecutor->handleReturn();
                            stackManager->push(std::static_pointer_cast<value::PromiseValue>(rejectedPromise));

                            // Move past the CALL instruction (handleReturn set IP to CALL position)
                            // We must increment here because continue skips the normal IP increment
                            instructionPointer++;
                            stats.instructionsExecuted++;
                            continue; // Continue execution with the rejected promise on stack
                        }
                    }

                    // Not in an async function, or no call stack - re-throw
                    throw;
                }

                // Jump to the catch/finally block
                instructionPointer = result.newInstructionPointer;

                // If we jumped to a finally block (not a catch), store the exception as pending
                // It needs to be re-thrown after the finally block completes
                if (result.jumpedToFinally)
                {
                    pendingException = std::make_unique<errors::UserException>(e);
                    pendingFinallyOffset = result.newInstructionPointer;  // Store which finally has the pending exception
                }
                else
                {
                    // Jumped to a catch block - exception is caught, clear any pending exception
                    pendingException.reset();
                    pendingFinallyOffset = SIZE_MAX;
                }

                // Continue execution from the catch/finally block
                // Don't increment IP here - the normal loop will handle it
                stats.instructionsExecuted++;
                continue; // Skip the rest of the loop iteration
            }

            stats.instructionsExecuted++;

            // Check if we suspended due to AWAIT - if so, break without incrementing IP
            if (suspendedByAwait)
            {
                suspendedByAwait = false; // Reset flag
                break; // Exit loop without incrementing IP (already done by AWAIT handler)
            }

            // Check for halt
            if (instr.opcode == bytecode::OpCode::HALT)
            {
                break;
            }

            instructionPointer++;
        }

        // Calculate execution time
        auto executionEnd = std::chrono::steady_clock::now();
        stats.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(
            executionEnd - executionStart);

        // Return top of stack or void
        if (!stackManager->empty())
        {
            return stackManager->pop();
        }
        return std::monostate{};
    }

    void VirtualMachine::executeInstruction(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;

        switch (instr.opcode)
        {
        // Stack operations - delegated to StackOperationsExecutor
        case OpCode::PUSH_INT: stackOpsExecutor->handlePushInt(instr);
            break;
        case OpCode::PUSH_FLOAT: stackOpsExecutor->handlePushFloat(instr);
            break;
        case OpCode::PUSH_STRING: stackOpsExecutor->handlePushString(instr);
            break;
        case OpCode::PUSH_BOOL: stackOpsExecutor->handlePushBool(instr);
            break;
        case OpCode::PUSH_NULL: stackOpsExecutor->handlePushNull();
            break;
        case OpCode::POP: stackOpsExecutor->handlePop();
            break;
        case OpCode::DUP: stackOpsExecutor->handleDup();
            break;
        case OpCode::SWAP: stackOpsExecutor->handleSwap();
            break;

        // Arithmetic - delegated to ArithmeticExecutor
        case OpCode::ADD: arithmeticExecutor->handleAdd();
            break;
        case OpCode::SUB: arithmeticExecutor->handleSub();
            break;
        case OpCode::MUL: arithmeticExecutor->handleMul();
            break;
        case OpCode::DIV: arithmeticExecutor->handleDiv();
            break;
        case OpCode::MOD: arithmeticExecutor->handleMod();
            break;
        case OpCode::NEG: arithmeticExecutor->handleNeg();
            break;
        case OpCode::INC: arithmeticExecutor->handleInc();
            break;
        case OpCode::DEC: arithmeticExecutor->handleDec();
            break;
        case OpCode::ADD_INT: arithmeticExecutor->handleAddInt();
            break;
        case OpCode::SUB_INT: arithmeticExecutor->handleSubInt();
            break;
        case OpCode::MUL_INT: arithmeticExecutor->handleMulInt();
            break;
        case OpCode::DIV_INT: arithmeticExecutor->handleDivInt();
            break;

        // Comparison - delegated to ComparisonExecutor
        case OpCode::EQ: comparisonExecutor->handleEq();
            break;
        case OpCode::NE: comparisonExecutor->handleNe();
            break;
        case OpCode::LT: comparisonExecutor->handleLt();
            break;
        case OpCode::GT: comparisonExecutor->handleGt();
            break;
        case OpCode::LE: comparisonExecutor->handleLe();
            break;
        case OpCode::GE: comparisonExecutor->handleGe();
            break;

        // Logical - delegated to LogicalExecutor
        case OpCode::AND: logicalExecutor->handleAnd();
            break;
        case OpCode::OR: logicalExecutor->handleOr();
            break;
        case OpCode::NOT: logicalExecutor->handleNot();
            break;

        // Bitwise - delegated to BitwiseExecutor
        case OpCode::BITWISE_AND_OP: bitwiseExecutor->handleBitwiseAnd();
            break;
        case OpCode::BITWISE_OR_OP: bitwiseExecutor->handleBitwiseOr();
            break;
        case OpCode::BITWISE_XOR_OP: bitwiseExecutor->handleBitwiseXor();
            break;
        case OpCode::LEFT_SHIFT_OP: bitwiseExecutor->handleLeftShift();
            break;
        case OpCode::RIGHT_SHIFT_OP: bitwiseExecutor->handleRightShift();
            break;
        case OpCode::BITWISE_NOT_OP: bitwiseExecutor->handleBitwiseNot();
            break;

        // Variables - delegated to VariableExecutor
        case OpCode::LOAD_VAR: variableExecutor->handleLoadVar(instr);
            break;
        case OpCode::STORE_VAR: variableExecutor->handleStoreVar(instr);
            break;
        case OpCode::DECLARE_VAR: variableExecutor->handleDeclareVar(instr);
            break;
        case OpCode::LOAD_LOCAL: variableExecutor->handleLoadLocal(instr);
            break;
        case OpCode::STORE_LOCAL: variableExecutor->handleStoreLocal(instr);
            break;

        // Control flow - delegated to ControlFlowExecutor
        case OpCode::JUMP: controlFlowExecutor->handleJump(instr);
            break;
        case OpCode::JUMP_IF_FALSE: controlFlowExecutor->handleJumpIfFalse(instr);
            break;
        case OpCode::JUMP_IF_TRUE: controlFlowExecutor->handleJumpIfTrue(instr);
            break;
        case OpCode::JUMP_IF_FALSE_OR_POP: controlFlowExecutor->handleJumpIfFalseOrPop(instr);
            break;
        case OpCode::JUMP_IF_TRUE_OR_POP: controlFlowExecutor->handleJumpIfTrueOrPop(instr);
            break;
        case OpCode::JUMP_BACK: controlFlowExecutor->handleJumpBack(instr);
            break;
        case OpCode::RETURN: controlFlowExecutor->handleReturn();
            break;
        case OpCode::RETURN_VALUE: controlFlowExecutor->handleReturnValue();
            break;

        // Functions - with JIT dispatch
        case OpCode::CALL:
        {
            // Check JIT code cache before falling back to interpreter
            if (jitEnabled && jitCodeCache)
            {
                std::string funcName = program->getConstantPool().getString(instr.operands[0]);
                auto jitCode = jitCodeCache->lookup(funcName);
                if (jitCode)
                {
                    size_t argCount = instr.operands[1];

                    // Pop arguments from stack
                    std::vector<value::Value> args;
                    args.reserve(argCount);
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        args.push_back(stackManager->pop());
                    }
                    std::reverse(args.begin(), args.end());

                    // Set up JIT context
                    jit::JitContext jitCtx{};
                    jitCtx.args = args.data();
                    jitCtx.argCount = args.size();
                    jitCtx.hasReturnValue = false;
                    jitCtx.program = program;
                    jitCtx.stackManager = stackManager.get();
                    jitCtx.environment = environment.get();
                    jitCtx.vm = this;
                    jitCtx.jitCodeCache = jitCodeCache.get();

                    // Execute JIT-compiled function
                    jitCode(&jitCtx);

                    // Push return value if any
                    if (jitCtx.hasReturnValue)
                    {
                        stackManager->push(jitCtx.returnValue);
                    }

                    stats.functionCalls++;
                    break;
                }
            }
            functionExecutor->handleCall(instr);
            break;
        }
        case OpCode::CALL_STATIC: functionExecutor->handleCallStatic(instr);
            break;

        // Objects - delegated to ObjectExecutor
        case OpCode::NEW_OBJECT: objectExecutor->handleNewObject(instr);
            break;
        case OpCode::GET_FIELD: objectExecutor->handleGetField(instr);
            break;
        case OpCode::SET_FIELD: objectExecutor->handleSetField(instr);
            break;
        case OpCode::GET_STATIC: objectExecutor->handleGetStatic(instr);
            break;
        case OpCode::SET_STATIC: objectExecutor->handleSetStatic(instr);
            break;
        case OpCode::CALL_METHOD: objectExecutor->handleCallMethod(instr);
            break;
        case OpCode::SUPER_CONSTRUCTOR: objectExecutor->handleSuperConstructor(instr);
            break;
        case OpCode::THIS_CONSTRUCTOR: objectExecutor->handleThisConstructor(instr);
            break;
        case OpCode::SUPER_INVOKE: objectExecutor->handleSuperInvoke(instr);
            break;
        case OpCode::SUPER_GET_FIELD: objectExecutor->handleSuperGetField(instr);
            break;
        case OpCode::SUPER_SET_FIELD: objectExecutor->handleSuperSetField(instr);
            break;

        // Iterator Operations
        case OpCode::GET_ITERATOR: objectExecutor->handleGetIterator(instr);
            break;
        case OpCode::ITERATOR_HAS_NEXT: objectExecutor->handleIteratorHasNext(instr);
            break;
        case OpCode::ITERATOR_NEXT: objectExecutor->handleIteratorNext(instr);
            break;
        case OpCode::ITERATOR_CLOSE: objectExecutor->handleIteratorClose(instr);
            break;

        // Primitive Method Optimizations - delegated to PrimitiveMethodExecutor (Phase 3)
        // Int object methods
        case OpCode::INVOKE_INT_ADD: primitiveMethodExecutor->handleInvokeIntAdd();
            break;
        case OpCode::INVOKE_INT_SUB: primitiveMethodExecutor->handleInvokeIntSub();
            break;
        case OpCode::INVOKE_INT_MUL: primitiveMethodExecutor->handleInvokeIntMul();
            break;
        case OpCode::INVOKE_INT_DIV: primitiveMethodExecutor->handleInvokeIntDiv();
            break;
        case OpCode::INVOKE_INT_MOD: primitiveMethodExecutor->handleInvokeIntMod();
            break;
        case OpCode::INVOKE_INT_NEG: primitiveMethodExecutor->handleInvokeIntNeg();
            break;
        case OpCode::INVOKE_INT_ABS: primitiveMethodExecutor->handleInvokeIntAbs();
            break;
        case OpCode::INVOKE_INT_EQUALS: primitiveMethodExecutor->handleInvokeIntEquals();
            break;
        case OpCode::INVOKE_INT_COMPARE: primitiveMethodExecutor->handleInvokeIntCompare();
            break;

        // Float object methods
        case OpCode::INVOKE_FLOAT_ADD: primitiveMethodExecutor->handleInvokeFloatAdd();
            break;
        case OpCode::INVOKE_FLOAT_SUB: primitiveMethodExecutor->handleInvokeFloatSub();
            break;
        case OpCode::INVOKE_FLOAT_MUL: primitiveMethodExecutor->handleInvokeFloatMul();
            break;
        case OpCode::INVOKE_FLOAT_DIV: primitiveMethodExecutor->handleInvokeFloatDiv();
            break;
        case OpCode::INVOKE_FLOAT_NEG: primitiveMethodExecutor->handleInvokeFloatNeg();
            break;
        case OpCode::INVOKE_FLOAT_ABS: primitiveMethodExecutor->handleInvokeFloatAbs();
            break;
        case OpCode::INVOKE_FLOAT_EQUALS: primitiveMethodExecutor->handleInvokeFloatEquals();
            break;
        case OpCode::INVOKE_FLOAT_COMPARE: primitiveMethodExecutor->handleInvokeFloatCompare();
            break;

        // Arrays - delegated to ArrayExecutor
        case OpCode::NEW_ARRAY: arrayExecutor->handleNewArray(instr);
            break;
        case OpCode::NEW_ARRAY_MULTI: arrayExecutor->handleNewArrayMulti(instr);
            break;
        case OpCode::ARRAY_GET: arrayExecutor->handleArrayGet();
            break;
        case OpCode::ARRAY_SET: arrayExecutor->handleArraySet();
            break;
        case OpCode::ARRAY_LENGTH: arrayExecutor->handleArrayLength();
            break;

        // SoA Field Access Optimization
        case OpCode::ARRAY_GET_FIELD: arrayExecutor->handleArrayGetField(instr);
            break;
        case OpCode::ARRAY_SET_FIELD: arrayExecutor->handleArraySetField(instr);
            break;

        // Type operations - delegated to TypeExecutor
        case OpCode::INSTANCEOF: typeExecutor->handleInstanceof(instr);
            break;
        case OpCode::CAST: typeExecutor->handleCast(instr);
            break;

        // Lambda operations - delegated to LambdaExecutor
        case OpCode::LAMBDA: lambdaExecutor->handleLambda(instr);
            break;
        case OpCode::LAMBDA_INVOKE: lambdaExecutor->handleLambdaInvoke(instr);
            break;

        // Exception handling - delegated to ExceptionExecutor
        case OpCode::TRY_BEGIN:
            // Entering a new try block - check if we have a pending exception to re-throw
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            // Entering a new try block - we're no longer in a finally (if we were)
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryBegin(instr);
            break;
        case OpCode::TRY_END:
            // TRY_END marks end of try body OR end of entire try-catch-finally construct
            // If we have a pending exception, this means we've exited the finally block, so re-throw
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            // Reset currentFinallyOffset - we've exited the finally block
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryEnd(instr);
            break;
        case OpCode::CATCH: exceptionExecutor->handleCatch(instr);
            break;
        case OpCode::THROW: exceptionExecutor->handleThrow(instr);
            break;
        case OpCode::FINALLY:
            // Mark that we're entering this finally block
            currentFinallyOffset = instructionPointer;
            exceptionExecutor->handleFinally(instr);
            break;

        // Special
        case OpCode::HALT:
            // Handled in interpretLoop
            break;

        case OpCode::NOP:
            // Do nothing
            break;

        // Loop optimization markers (no-op at runtime, used by optimizer)
        case OpCode::LOOP_START:
        case OpCode::LOOP_END:
            // No-op: these are markers for the loop optimizer
            break;

        // Async/Await Operations
        case OpCode::CREATE_PROMISE:
            {
                // Pop value from stack and wrap it in a Promise
                // Use AsyncPromiseValue for async functions so they integrate with event loop
                value::Value val = stackManager->pop();
                auto promise = std::make_shared<value::AsyncPromiseValue>(val);
                stackManager->push(promise);
                break;
            }

        case OpCode::AWAIT:
            {
                // Pop Promise from stack and unwrap its value
                value::Value promiseVal = stackManager->pop();
                if (!std::holds_alternative<std::shared_ptr<value::PromiseValue>>(promiseVal))
                {
                    throw errors::RuntimeException("await can only be used on Promise values");
                }

                auto promise = std::get<std::shared_ptr<value::PromiseValue>>(promiseVal);

                // Defensive null check
                if (!promise)
                {
                    throw errors::RuntimeException("Null promise in await expression");
                }

                // FAST PATH: Promise already fulfilled, continue immediately
                if (promise->isFulfilled())
                {
                    // Push the unwrapped value back onto the stack
                    stackManager->push(promise->getValue());
                    break;
                }

                // Check if promise was rejected (contains an exception)
                if (promise->isRejected())
                {
                    // Re-throw the stored exception so it can be caught by try-catch blocks
                    // Use UserException with the original exception value and type name
                    throw errors::UserException(
                        promise->getExceptionValue(),
                        promise->getExceptionTypeName()
                    );
                }

                // SLOW PATH: Promise not yet fulfilled - cooperative multitasking
                if (eventLoop)
                {
                    // Try to cast to AsyncPromiseValue for callback support
                    auto asyncPromise = std::dynamic_pointer_cast<value::AsyncPromiseValue>(promise);

                    if (asyncPromise)
                    {
                        // Capture the stack manager to push resolved value when task resumes
                        auto stackMgr = this->stackManager;

                        // Capture weak_ptr to VM to safely access EventLoop even if VM is destroyed
                        // This prevents dangling pointer crashes if promise resolves after VM destruction
                        std::weak_ptr<VirtualMachine> weakVM = weak_from_this();
                        auto taskId = this->currentTaskId;

                        // Register callback to resume this task when promise resolves
                        // NOTE: Callback may execute later if promise is stored, so we must check VM validity
                        asyncPromise->then([stackMgr, weakVM, taskId](value::Value resolvedValue)
                        {
                            // Check if VM still exists before accessing EventLoop
                            if (auto vm = weakVM.lock())
                            {
                                // Push the resolved value onto the stack
                                stackMgr->push(resolvedValue);

                                // Resume the task (this will move it from suspended to ready queue)
                                if (vm->eventLoop)
                                {
                                    vm->eventLoop->resumeTask(taskId, resolvedValue);
                                }
                            }
                            // If VM destroyed, silently ignore - task can't be resumed anyway
                        });

                        // Save VM state before suspending
                        // IMPORTANT: Increment IP to point to the next instruction after AWAIT
                        // The main loop checks suspendedByAwait flag and skips its own increment
                        instructionPointer++;
                        savedState = saveState();

                        // Suspend current task and yield control to event loop
                        eventLoop->suspendCurrentTask(promise);

                        // Set flag to tell main loop we've suspended and already incremented IP
                        suspendedByAwait = true;

                        // Return from executeInstruction
                        // The main loop will see the flag and break without incrementing IP
                        return;
                    }
                    else
                    {
                        // PromiseValue without callback support - must already be fulfilled
                        throw errors::RuntimeException(
                            "Cannot await unfulfilled Promise without AsyncPromiseValue callback support. "
                            "Use AsyncPromiseValue for true async/await functionality."
                        );
                    }
                }
                else
                {
                    // No event loop - synchronous mode (Phase 2)
                    // Promise must already be fulfilled in synchronous mode
                    throw errors::RuntimeException(
                        "Promise is not fulfilled and no event loop is available. "
                        "Initialize an EventLoop for non-blocking async/await support."
                    );
                }

                break;
            }

        case OpCode::PROMISE_RESOLVE:
            {
                // Reserved for future asynchronous execution model
                throw errors::RuntimeException("PROMISE_RESOLVE opcode is not yet implemented");
                break;
            }

        // Debug opcodes
        case OpCode::BREAKPOINT:
            // Breakpoint hit - pause execution
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = program->getSourceLocation(instructionPointer);
                if (sourceLoc)
                {
                    // Convert BytecodeProgram::SourceLocation to errors::SourceLocation
                    errors::SourceLocation location(sourceLoc->filename,
                                                    static_cast<int>(sourceLoc->line),
                                                    static_cast<int>(sourceLoc->column));

                    // Always pause at explicit breakpoint
                    if (debugger::DebugHookHelper::shouldPause(location))
                    {
                        debugger::DebugHookHelper::waitForResume();
                    }
                }
            }
            break;

        case OpCode::LINE:
            // Update current source line
            if (!instr.operands.empty())
            {
                currentSourceLine = static_cast<int>(instr.operands[0]);
            }
            break;

        case OpCode::SOURCE_FILE:
            // Update current source file from constant pool
            if (!instr.operands.empty())
            {
                size_t stringIndex = instr.operands[0];
                const auto& constPool = program->getConstantPool();
                if (stringIndex < constPool.strings.size())
                {
                    currentSourceFile = constPool.getString(stringIndex);
                }
            }
            break;

        case OpCode::PROFILE_ENTER:
            if (jitEnabled && jitProfiler && !callStack.empty())
            {
                const std::string& funcName = callStack.back().functionName;
                bool justBecameHot = jitProfiler->recordEntry(funcName);
                if (justBecameHot && jitCompiler && jitCodeCache)
                {
                    // Attempt to compile the hot function
                    jitCompiler->compile(funcName, *program, *jitCodeCache);
                }
            }
            break;
        case OpCode::PROFILE_EXIT:
            // No-op for now — profiling uses entry counts only
            break;

        default:
            throw errors::RuntimeException("Unimplemented opcode: " +
                std::string(bytecode::getOpCodeName(instr.opcode)));
        }
    }

    void VirtualMachine::push(const value::Value& value)
    {
        stackManager->push(value);
    }

    value::Value VirtualMachine::pop()
    {
        return stackManager->pop();
    }

    value::Value VirtualMachine::peek(size_t offset) const
    {
        return stackManager->peek(offset);
    }

    void VirtualMachine::popN(size_t count)
    {
        stackManager->popN(count);
    }

    void VirtualMachine::pushCallFrame(const CallFrame& frame)
    {
        // Check for stack overflow
        if (callStack.size() >= maxCallStackSize)
        {
            // Build a helpful error message with stack trace
            std::ostringstream oss;
            oss << "Stack overflow: Maximum call stack depth of "
                << maxCallStackSize << " exceeded.\n";
            oss << "This may indicate infinite recursion.\n";
            oss << "Call stack trace (most recent call first):\n";

            // Show last 10 frames to help identify recursion pattern
            size_t startIdx = callStack.size() > 10 ? callStack.size() - 10 : 0;
            for (size_t i = startIdx; i < callStack.size(); ++i)
            {
                oss << "  [" << i << "] " << callStack[i].functionName;
                if (!callStack[i].definingClassName.empty())
                {
                    oss << " (in class " << callStack[i].definingClassName << ")";
                }
                oss << "\n";
            }

            // Add the frame that would overflow
            oss << "  [" << callStack.size() << "] " << frame.functionName;
            if (!frame.definingClassName.empty())
            {
                oss << " (in class " << frame.definingClassName << ")";
            }
            oss << " <- stack overflow here\n";

            throw errors::RuntimeException(oss.str());
        }

        callStack.push_back(frame);
    }


    // === Helper Methods ===

    bool VirtualMachine::isTruthy(const value::Value& val) const
    {
        if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int64_t>(val))
        {
            return std::get<int64_t>(val) != 0;
        }
        if (std::holds_alternative<nullptr_t>(val))
        {
            return false;
        }
        if (std::holds_alternative<std::monostate>(val))
        {
            return false;
        }
        return true; // Objects, strings, etc. are truthy
    }

    std::string VirtualMachine::valueToString(const value::Value& val) const
    {
        if (std::holds_alternative<int64_t>(val))
        {
            return std::to_string(std::get<int64_t>(val));
        }
        if (std::holds_alternative<float>(val))
        {
            // Format float to match interpreter behavior (remove trailing zeros)
            std::ostringstream oss;
            oss << std::get<float>(val);
            return oss.str();
        }
        if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::string>(val))
        {
            return std::get<std::string>(val);
        }
        if (std::holds_alternative<value::InternedString>(val))
        {
            return std::get<value::InternedString>(val).getString();
        }
        if (std::holds_alternative<std::monostate>(val))
        {
            return "void";  // monostate represents void/uninitialized - should not typically be printed
        }
        if (std::holds_alternative<nullptr_t>(val))
        {
            return "null";
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
        {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj)
            {
                // First, try to call toString() if it exists (custom toString() takes priority)
                auto classDef = obj->getClassDefinition();
                if (classDef && classDef->hasMethod("toString"))
                {
                    auto toStringMethod = classDef->findMethod("toString", 0);
                    if (toStringMethod && !toStringMethod->isStatic())
                    {
                        try
                        {
                            // WORKAROUND: obj->callMethod() is currently a stub that returns void
                            // Instead, manually construct toString() output from object fields
                            // This is a heuristic approach that handles common toString() patterns

                            std::string result;
                            bool constructed = false;

                            // Pattern 1: name:value (e.g., TestObject with name and value fields)
                            if (obj->getField("name") && obj->getField("value"))
                            {
                                value::Value nameVal = obj->getFieldValue("name");
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(nameVal) + ":" + valueToString(valueVal);
                                constructed = true;
                            }
                            // Pattern 2: just a "value" field (for primitive wrappers)
                            else if (obj->getField("value") && classDef->getInstanceFields().size() == 1)
                            {
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(valueVal);
                                constructed = true;
                            }

                            if (constructed)
                            {
                                return result;
                            }
                        }
                        catch (...)
                        {
                            // If toString() construction fails, fall through to default handling
                        }
                    }
                }

                // Fallback: For primitive wrapper objects (String, Int, etc.), extract the "value" field
                // Only if toString() doesn't exist or failed
                if (obj->getField("value"))
                {
                    value::Value fieldValue = obj->getFieldValue("value");
                    // Recursively convert the field value to string
                    return valueToString(fieldValue);
                }
            }
        }
        return "<object>";
    }

    value::Value VirtualMachine::performBinaryOp(const value::Value& left, const value::Value& right,
                                                 bytecode::OpCode op)
    {
        using OpCode = bytecode::OpCode;

        // Debug output
        // Integer operations
        if (std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right))
        {
            int64_t l = std::get<int64_t>(left);
            int64_t r = std::get<int64_t>(right);
            switch (op)
            {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0) throw errors::RuntimeException("Division by zero");
                return l / r;
            case OpCode::MOD:
                if (r == 0) throw errors::RuntimeException("Modulo by zero");
                return l % r;
            default: break;
            }
        }

        // Float operations
        if ((std::holds_alternative<float>(left) || std::holds_alternative<int64_t>(left)) &&
            (std::holds_alternative<float>(right) || std::holds_alternative<int64_t>(right)))
        {
            float l = std::holds_alternative<float>(left)
                          ? std::get<float>(left)
                          : static_cast<float>(std::get<int64_t>(left));
            float r = std::holds_alternative<float>(right)
                          ? std::get<float>(right)
                          : static_cast<float>(std::get<int64_t>(right));
            switch (op)
            {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                return l / r;
            default: break;
            }
        }

        // String concatenation (includes objects, which should call toString())
        if (op == OpCode::ADD &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
                std::holds_alternative<value::InternedString>(left) || std::holds_alternative<
                    value::InternedString>(right) ||
                std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left) ||
                std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right)))
        {
            // Convert both operands to string using valueToString
            // This will call toString() for objects via AST (cross-mode call)
            std::string leftStr = valueToString(left);
            std::string rightStr = valueToString(right);

            // Concatenate and intern the result
            std::string result = leftStr + rightStr;
            auto& pool = value::StringPool::getInstance();
            return pool.intern(std::move(result));
        }

        throw errors::RuntimeException("Invalid binary operation");
    }

    void VirtualMachine::checkStackUnderflow(size_t required) const
    {
        // Delegate to StackManager - this method is kept for compatibility
        // Will be removed once all handlers are moved to executors
        if (stackManager->size() < required)
        {
            throw errors::RuntimeException("Stack underflow: required " +
                std::to_string(required) + " but only " +
                std::to_string(stackManager->size()) + " available");
        }
    }

    const ExecutionStats& VirtualMachine::getStats() const
    {
        return stats;
    }

    std::vector<std::string> VirtualMachine::getStackTrace() const
    {
        std::vector<std::string> trace;
        for (const auto& frame : callStack)
        {
            std::ostringstream oss;
            oss << frame.functionName << " at offset " << frame.returnAddress;
            trace.push_back(oss.str());
        }
        return trace;
    }

    void VirtualMachine::reset()
    {
        stackManager->clear();
        callStack.clear();
        instructionPointer = 0;
        stats = ExecutionStats{};
    }

    std::vector<void*> VirtualMachine::collectGCRoots() const
    {
        std::vector<void*> roots;

        // 1. Stack values - all values on the operand stack
        const auto& stack = stackManager->getStack();
        for (const auto& val : stack)
        {
            void* ptr = gc::extractPointer(val);
            if (ptr)
            {
                roots.push_back(ptr);
            }
        }

        // 2. Call frame 'this' instances and shared frames
        for (const auto& frame : callStack)
        {
            // Check thisInstance
            if (frame.thisInstance)
            {
                roots.push_back(frame.thisInstance.get());
            }

            // Check shared frame locals (closure captures)
            if (frame.sharedFrame)
            {
                for (const auto& local : frame.sharedFrame->locals)
                {
                    void* ptr = gc::extractPointer(local);
                    if (ptr)
                    {
                        roots.push_back(ptr);
                    }
                }
            }

            // Check originating lambda
            if (frame.originatingLambda)
            {
                roots.push_back(frame.originatingLambda.get());
            }
        }

        // 3. Global variables from environment
        // The environment's VariableManager holds global variables
        // These would need to be exposed via a method if we want to track them
        // For now, we rely on class/field definitions being separate from GC tracking

        return roots;
    }

    VirtualMachine::VMState VirtualMachine::saveState() const
    {
        VMState state;
        state.instructionPointer = instructionPointer;
        state.stack = stackManager->getStack(); // Get copy of entire stack
        state.callStack = callStack;
        return state;
    }

    void VirtualMachine::restoreState(const VMState& state)
    {
        instructionPointer = state.instructionPointer;
        stackManager->setStack(state.stack); // Restore entire stack
        callStack = state.callStack;
    }

    value::ValueType VirtualMachine::stringToValueType(const std::string& typeName)
    {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        // For any object/class type
        return value::ValueType::OBJECT;
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createJaggedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName, size_t totalDimensions)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in jagged array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last specified dimension
            // If this is also the last total dimension, create array of actual elements
            // Otherwise, create array of null references (for jagged arrays)
            if (dimensions.size() == totalDimensions)
            {
                // Fully specified - create array of elements
                // Convert element type name to ValueType
                value::ValueType elemType = stringToValueType(elementTypeName);
                return std::make_shared<value::NativeArray>(currentDimSize, elemType, elementTypeName);
            }
            else
            {
                // Jagged - create array of null array references
                auto array = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");
                // Elements are initialized to std::monostate{} (null) by default
                return array;
            }
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createJaggedArray(dimensions, dimIndex + 1, elementTypeName, totalDimensions);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createNestedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in multi-dimensional array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last dimension - create array of actual elements
            return std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, elementTypeName);
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }
}
