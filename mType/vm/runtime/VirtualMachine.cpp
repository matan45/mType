#include "VirtualMachine.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <sstream>
// All executor headers needed for unique_ptr<Executor> default destructor.
#include "executors/ArithmeticExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/BitwiseExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/ExceptionExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/PrimitiveMethodExecutor.hpp"
#include "executors/StackOperationsExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "utils/ExceptionHandler.hpp"
#include "utils/InteropExceptionDecorator.hpp"
#include "../../constants/ExecutionMode.hpp"
#include "../../errors/FunctionNotFoundException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "../../gc/GC.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"

namespace vm::runtime
{
    VirtualMachine::VirtualMachine(std::shared_ptr<environment::Environment> env,
                                   size_t maxStackDepth)
        : program(nullptr)
          , stackManager(std::make_shared<StackManager>())
          , maxCallStackSize(maxStackDepth == 0 ? constants::vm::DEFAULT_MAX_CALL_STACK_SIZE : maxStackDepth)
          , instructionPointer(0)
          , environment(std::move(env))
          , eventLoop(nullptr)
          , currentTaskId(0)
          , suspendedByAwait(false)
          , debuggingEnabled(false)
          , currentSourceFile("")
          , currentSourceLine(0)
          , currentFinallyOffset(SIZE_MAX)
          , pendingFinallyOffset(SIZE_MAX)
          , jitEnabled(false)
          , icEnabled(false)
    {
        callStack.reserve(256);

        if (!gc::GC::isInitialized())
        {
            gc::GC::initialize();
        }

        // Wire up root collector so GC can scan VM stack and call frames.
        if (auto* coordinator = gc::GC::get())
        {
            coordinator->setRootCollector([this]() { return collectGCRoots(); });
        }

        // Executors are initialized in execute() once a program is bound,
        // because ExecutionContext requires a valid program pointer.
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

        if (savedState.has_value())
        {
            restoreState(savedState.value());
            savedState.reset();
        }
        else
        {
            instructionPointer = program->getEntryPoint();
            executionStart = std::chrono::steady_clock::now();
            stats = ExecutionStats{};

            // Create an initial call frame for the implicit "main" function
            // so global scope variables can be captured by lambdas.
            // MYT-197: pre-warm the sentinel handle once at bind time —
            // subsequent equality checks in the hot interpretation path are
            // integer compares.
            scriptMainHandle = program->internFrameName("__script_main__");
            CallFrame mainFrame;
            mainFrame.returnAddress = program->getInstructionCount();
            mainFrame.frameBase = 0;
            mainFrame.localBase = 0;
            mainFrame.functionName = scriptMainHandle;
            mainFrame.thisInstance = nullptr;
            mainFrame.definingClassName = "";
            callStack.push_back(mainFrame);
        }

        try
        {
            value::Value result = interpretLoop();

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

            for (const auto& arg : args)
            {
                push(arg);
            }

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

    const ExecutionStats& VirtualMachine::getStats() const
    {
        return stats;
    }
}
