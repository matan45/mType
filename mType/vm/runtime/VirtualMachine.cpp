#include "VirtualMachine.hpp"
// All executor headers needed for unique_ptr<Executor> default destructor
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
#include "executors/PrimitiveMethodExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "utils/ExceptionHandler.hpp"
#include "utils/InteropExceptionDecorator.hpp"
#include "../../errors/FunctionNotFoundException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../constants/ExecutionMode.hpp"
#include "../../gc/GC.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"
#include <chrono>
#include <sstream>

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
          , icEnabled(false)
    {
        callStack.reserve(256);

        // Initialize garbage collector if not already initialized
        if (!gc::GC::isInitialized())
        {
            gc::GC::initialize();
        }

        // Wire up root collector so GC can scan VM stack and call frames
        if (auto* coordinator = gc::GC::get())
        {
            coordinator->setRootCollector([this]() { return collectGCRoots(); });
        }

        // Note: Executors will be initialized in execute() when program is available
        // because ExecutionContext requires a valid program pointer
    }

    VirtualMachine::~VirtualMachine() = default;

    void VirtualMachine::setPendingTypeArgs(std::unordered_map<std::string, std::string> bindings)
    {
        // MYT-228: stage type-arg bindings into the ExecutionContext scratch
        // slot. The next pushCallFrame consumes them into the new frame.
        // Pulls a map from the pool and moves the caller's bindings into it
        // so the storage is recycled across generic calls.
        if (!executionCtx) return;
        auto& target = executionCtx->pendingTypeArgs.acquireFresh();
        target = std::move(bindings);
    }

    TypeArgMap& VirtualMachine::beginPendingTypeArgs()
    {
        // MYT-228: caller-populated pool map. acquireFresh clears any prior
        // pending map and returns a reference for in-place insertion.
        return executionCtx->pendingTypeArgs.acquireFresh();
    }

    void VirtualMachine::setJitEnabled(bool enabled)
    {
        jitEnabled = enabled;
        if (enabled && !jitProfiler)
        {
            jitProfiler  = std::make_unique<jit::JitProfiler>();
            jitCodeCache = std::make_unique<jit::JitCodeCache>();
            jitCompiler  = std::make_unique<jit::JitCompiler>();
            osrManager   = std::make_unique<jit::OSRManager>();
        }
        if (enabled) setICEnabled(true);
    }

    void VirtualMachine::setICEnabled(bool enabled)
    {
        icEnabled = enabled;
        if (enabled && !inlineCacheTable)
        {
            inlineCacheTable = std::make_unique<jit::ic::InlineCacheTable>();
            typeFeedbackCollector = std::make_unique<jit::ic::TypeFeedbackCollector>(*inlineCacheTable);
        }
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
            // Use "__script_main__" to match the expected name for global scope access checks.
            // MYT-197: pre-warm the sentinel handle once at bind time — subsequent
            // equality checks in the hot interpretation path are integer compares.
            scriptMainHandle = program->internFrameName("__script_main__");
            CallFrame mainFrame;
            mainFrame.returnAddress = program->getInstructionCount(); // Return past end (halt)
            mainFrame.frameBase = 0;
            mainFrame.localBase = 0;
            mainFrame.functionName = scriptMainHandle;
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

            // Pop the main frame if it's still on the stack.
            // MYT-197: integer compare against the pre-warmed sentinel.
            // MYT-208: release stack-promoted allocations before pop.
            if (!callStack.empty() && callStack.back().functionName == scriptMainHandle) {
                callStack.back().releaseStackObjects();
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
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;

        try
        {
            if (!program)
            {
                throw errors::RuntimeException("No program loaded");
            }

            auto* funcMetadata = program->getFunction(functionName);
            if (!funcMetadata)
            {
                // MYT-46: typed throw → MT-E1010 NameFunctionNotFound.
                throw errors::FunctionNotFoundException(functionName);
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
        catch (errors::ScriptException& e)
        {
            // MYT-46: decorate using the LIVE callStack BEFORE restoring the
            // saved one — the live stack still contains any frame pushed by
            // interpretLoop. The decorator no-ops on empty stacks.
            utils::decorateFromCallStack(e, callStack, program);
            instructionPointer = savedIP;
            callStack = savedCallStack;
            throw;
        }
        catch (...)
        {
            instructionPointer = savedIP;
            callStack = savedCallStack;
            throw;
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
            auto frameNameOf = [&](const CallFrame& f) -> const std::string& {
                const bytecode::BytecodeProgram* p =
                    (f.programIndex < loadedPrograms.size())
                        ? loadedPrograms[f.programIndex]
                        : program;
                return p->getFrameName(f.functionName);
            };
            size_t startIdx = callStack.size() > 10 ? callStack.size() - 10 : 0;
            for (size_t i = startIdx; i < callStack.size(); ++i)
            {
                oss << "  [" << i << "] " << frameNameOf(callStack[i]);
                if (!callStack[i].definingClassName.empty())
                {
                    oss << " (in class " << callStack[i].definingClassName << ")";
                }
                oss << "\n";
            }

            // Add the frame that would overflow
            oss << "  [" << callStack.size() << "] " << frameNameOf(frame);
            if (!frame.definingClassName.empty())
            {
                oss << " (in class " << frame.definingClassName << ")";
            }
            oss << " <- stack overflow here\n";

            throw errors::RuntimeException(oss.str());
        }

        callStack.push_back(frame);
    }

    void VirtualMachine::popCallStack()
    {
        if (!callStack.empty())
        {
            // MYT-208: release stack-promoted allocations before pop.
            callStack.back().releaseStackObjects();
            callStack.pop_back();
        }
    }


    // === Helper Methods ===

    const ExecutionStats& VirtualMachine::getStats() const
    {
        return stats;
    }

    std::vector<std::string> VirtualMachine::getStackTrace() const
    {
        std::vector<std::string> trace;
        for (const auto& frame : callStack)
        {
            const bytecode::BytecodeProgram* p =
                (frame.programIndex < loadedPrograms.size())
                    ? loadedPrograms[frame.programIndex]
                    : program;
            std::ostringstream oss;
            oss << p->getFrameName(frame.functionName)
                << " at offset " << frame.returnAddress;
            trace.push_back(oss.str());
        }
        return trace;
    }

    void VirtualMachine::reset()
    {
        stackManager->clear();
        // MYT-208: release every frame's stack-promoted allocations before
        // clearing callStack — otherwise the pool leaks recyclable slots
        // every time reset() is invoked (e.g. uncaught-exception cleanup).
        for (auto& frame : callStack)
        {
            frame.releaseStackObjects();
        }
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

            // MYT-208: stack-promoted `this` (NEW_STACK ctor) and frame-owned
            // raw allocations. These are not GC-registered, but their fields
            // may hold heap references that the cycle detector must keep alive
            // while the owning frame is live. Inline array iteration over
            // [0, stackObjectsCount) — entries beyond count are uninitialised.
            if (frame.thisInstanceRaw)
            {
                roots.push_back(frame.thisInstanceRaw);
            }
            for (size_t i = 0; i < frame.stackObjectsCount; ++i)
            {
                if (frame.stackObjects[i])
                {
                    roots.push_back(frame.stackObjects[i]);
                }
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

}
