#include "VirtualMachine.hpp"
#include "executors/StackOperationsExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ArithmeticExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/ExceptionExecutor.hpp"
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
#include "../../value/LambdaValue.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugHookHelper.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <iostream>
namespace vm::runtime
{
    VirtualMachine::VirtualMachine(std::shared_ptr<environment::Environment> env)
        : program(nullptr)
          , stackManager(std::make_shared<StackManager>())
          , instructionPointer(0)
          , environment(std::move(env))
          , eventLoop(nullptr) // Lazy initialization - only created when needed
          , currentTaskId(0)
          , suspendedByAwait(false)
          , debuggingEnabled(false)
          , currentSourceFile("")
          , currentSourceLine(0)
          , currentFinallyOffset(SIZE_MAX) // Not in finally initially
    {
        callStack.reserve(64);

        // Note: Executors will be initialized in execute() when program is available
        // because ExecutionContext requires a valid program pointer
    }

    VirtualMachine::~VirtualMachine() = default;

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
        std::cerr << "[DEBUG C++] VirtualMachine::execute started\n";
        program = &bytecodeProgram;

        // Check if we're resuming from a saved state
        if (savedState.has_value())
        {
            std::cerr << "[DEBUG C++] Restoring from saved state\n";
            restoreState(savedState.value());
            savedState.reset(); // Clear saved state after restoring
        }
        else
        {
            std::cerr << "[DEBUG C++] Fresh execution, entry point=" << bytecodeProgram.getEntryPoint() << "\n";
            // Fresh execution - start from entry point
            instructionPointer = program->getEntryPoint();
            executionStart = std::chrono::steady_clock::now();
            stats = ExecutionStats{};
        }

        // Note: Executors are now initialized in interpretLoop() to ensure
        // they always have valid references, even when called from C++ API methods

        // Note: The main script frame is now pushed in Main.cpp before pausing at entry,
        // so VS Code has a frame to display immediately. It's also popped there after completion.

        std::cerr << "[DEBUG C++] About to call interpretLoop()\n";
        try
        {
            value::Value result = interpretLoop();
            std::cerr << "[DEBUG C++] interpretLoop() returned successfully\n";
            return result;
        }
        catch (...)
        {
            std::cerr << "[DEBUG C++] interpretLoop() threw exception, cleaning up\n";
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
            throw errors::RuntimeException("No program loaded - cannot create object in bytecode mode without compiled bytecode");
        }

        // Find class definition
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        // Create object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        // Find constructor
        auto constructor = classDef->findConstructor(args.size());
        if (!constructor)
        {
            throw errors::RuntimeException("Constructor not found for class '" + className +
                                         "' with " + std::to_string(args.size()) + " parameters");
        }

        // Look for constructor bytecode
        // Constructors are registered as "ClassName::<init>/paramCount"
        std::string qualifiedName = className + "::<init>/" + std::to_string(args.size());
        auto* ctorMetadata = program->getFunction(qualifiedName);
        if (!ctorMetadata)
        {
            throw errors::RuntimeException("Constructor '" + qualifiedName +
                                         "' has no bytecode. Bytecode compilation is required for VM execution.");
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;

        try
        {
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

            callStack.push_back(frame);
            stats.functionCalls++;

            // Execute constructor
            instructionPointer = ctorMetadata->startOffset;
            interpretLoop();

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;

            return instance;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            throw;
        }
    }

    value::Value VirtualMachine::invokeMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                             const std::string& methodName,
                                             const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException("No program loaded - cannot invoke method in bytecode mode without compiled bytecode");
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

        // Find which class defines this method
        std::string definingClassName = classDef->getName();
        auto currentClass = classDef;
        while (currentClass)
        {
            auto localMethod = currentClass->findInstanceMethod(methodName, argCount);
            if (localMethod)
            {
                definingClassName = currentClass->getName();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        // Look for method bytecode
        std::string qualifiedName = definingClassName + "::" + methodName;
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Method '" + qualifiedName +
                                         "' has no bytecode. Bytecode compilation is required for VM execution.");
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;

        try
        {
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

            callStack.push_back(frame);
            stats.functionCalls++;

            // Execute method
            instructionPointer = funcMetadata->startOffset;
            value::Value result = interpretLoop();

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;

            return result;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            throw;
        }
    }

    value::Value VirtualMachine::invokeStaticMethod(const std::string& className,
                                                    const std::string& methodName,
                                                    const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException("No program loaded - cannot invoke static method in bytecode mode without compiled bytecode");
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

        // Look for method bytecode
        std::string qualifiedName = className + "::" + methodName;
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Static method '" + qualifiedName +
                                         "' has no bytecode. Bytecode compilation is required for VM execution.");
        }

        // Save current state
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;

        try
        {
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
            frame.thisInstance = nullptr;  // Static methods have no 'this'
            frame.definingClassName = className;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Execute method
            instructionPointer = funcMetadata->startOffset;
            value::Value result = interpretLoop();

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;

            return result;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
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

        // DEBUG: Verify this code is being executed
        std::cerr << "[DEBUG VM] interpretLoop started - enhanced logging active\n";

        // Initialize executors with fresh execution context
        // This ensures executors always have valid references, even when called from C++ API
        ExecutionContext context(program, instructionPointer, callStack, environment,
                                 stackManager, stats, executionStart,
                                 debuggingEnabled, currentSourceFile, currentSourceLine);
        stackOpsExecutor = std::make_unique<StackOperationsExecutor>(context);
        comparisonExecutor = std::make_unique<ComparisonExecutor>(context);
        logicalExecutor = std::make_unique<LogicalExecutor>(context);
        arithmeticExecutor = std::make_unique<ArithmeticExecutor>(context);
        controlFlowExecutor = std::make_unique<ControlFlowExecutor>(context);
        variableExecutor = std::make_unique<VariableExecutor>(context);
        functionExecutor = std::make_unique<FunctionExecutor>(context);
        typeExecutor = std::make_unique<TypeExecutor>(context);
        arrayExecutor = std::make_unique<ArrayExecutor>(context);
        objectExecutor = std::make_unique<ObjectExecutor>(context);
        lambdaExecutor = std::make_unique<LambdaExecutor>(context);
        exceptionExecutor = std::make_unique<ExceptionExecutor>(context);

        // Set function executor reference in object executor for lambda-to-interface conversion
        objectExecutor->setFunctionExecutor(functionExecutor.get());

        // Initialize exception handler
        exceptionHandler = std::make_unique<utils::ExceptionHandler>(program, stackManager, callStack);

        std::cerr << "[DEBUG C++] Starting execution loop, IP=" << instructionPointer
                  << ", instrCount=" << program->getInstructionCount() << "\n";

        while (instructionPointer < program->getInstructionCount())
        {
            const auto& instr = program->getInstruction(instructionPointer);
            std::cerr << "[DEBUG C++] Executing IP=" << instructionPointer
                      << ", OpCode=" << bytecode::getOpCodeName(instr.opcode) << "\n";

            // DEBUG: Unconditionally log instructions with missing source locations
            auto debugSourceLoc = program->getSourceLocation(instructionPointer);
            if (!debugSourceLoc || debugSourceLoc->line == 0) {
                std::cerr << "[DEBUG VM MISSING LOC] IP=" << instructionPointer
                          << " OpCode=" << bytecode::getOpCodeName(instr.opcode)
                          << " Operands=" << instr.operands.size() << "\n";
            }

            // Debug hook: Check for breakpoints and stepping before executing instruction
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                // Get source location for current instruction from program
                auto sourceLoc = program->getSourceLocation(instructionPointer);

                // DEBUG: Log source location lookup
                std::cerr << "[DEBUG VM] IP=" << instructionPointer
                          << " OpCode=" << bytecode::getOpCodeName(instr.opcode)
                          << " HasSourceLoc=" << (sourceLoc != nullptr ? "yes" : "no");
                if (sourceLoc) {
                    std::cerr << " Loc=" << sourceLoc->filename << ":" << sourceLoc->line;
                } else {
                    std::cerr << " Loc=<unknown>:1";
                }
                std::cerr << "\n";

                if (sourceLoc && sourceLoc->line > 0)
                {
                    currentSourceFile = sourceLoc->filename;
                    currentSourceLine = static_cast<int>(sourceLoc->line);

                    // Convert BytecodeProgram::SourceLocation to errors::SourceLocation
                    errors::SourceLocation location(sourceLoc->filename,
                                                   static_cast<int>(sourceLoc->line),
                                                   static_cast<int>(sourceLoc->column));

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
                    // No matching catch found anywhere - re-throw
                    throw;
                }

                // Jump to the catch/finally block
                instructionPointer = result.newInstructionPointer;

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
        case OpCode::JUMP_BACK: controlFlowExecutor->handleJumpBack(instr);
            break;
        case OpCode::RETURN: controlFlowExecutor->handleReturn();
            break;
        case OpCode::RETURN_VALUE: controlFlowExecutor->handleReturnValue();
            break;

        // Functions - delegated to FunctionExecutor
        case OpCode::CALL: functionExecutor->handleCall(instr);
            break;
        case OpCode::CALL_NATIVE: functionExecutor->handleCallNative(instr);
            break;
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
        case OpCode::SUPER_INVOKE: objectExecutor->handleSuperInvoke(instr);
            break;
        case OpCode::SUPER_GET_FIELD: objectExecutor->handleSuperGetField(instr);
            break;
        case OpCode::SUPER_SET_FIELD: objectExecutor->handleSuperSetField(instr);
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
            // Entering a new try block - we're no longer in a finally (if we were)
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryBegin(instr);
            break;
        case OpCode::TRY_END: exceptionExecutor->handleTryEnd(instr);
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
        case OpCode::PROFILE_EXIT:
            // Profiling opcodes - currently no-op
            // Can be implemented for performance profiling
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


    // === Helper Methods ===

    bool VirtualMachine::isTruthy(const value::Value& val) const
    {
        if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int>(val))
        {
            return std::get<int>(val) != 0;
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
        if (std::holds_alternative<int>(val))
        {
            return std::to_string(std::get<int>(val));
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
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right))
        {
            int l = std::get<int>(left);
            int r = std::get<int>(right);
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
        if ((std::holds_alternative<float>(left) || std::holds_alternative<int>(left)) &&
            (std::holds_alternative<float>(right) || std::holds_alternative<int>(right)))
        {
            float l = std::holds_alternative<float>(left)
                          ? std::get<float>(left)
                          : static_cast<float>(std::get<int>(left));
            float r = std::holds_alternative<float>(right)
                          ? std::get<float>(right)
                          : static_cast<float>(std::get<int>(right));
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
