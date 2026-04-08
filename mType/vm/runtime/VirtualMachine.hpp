#pragma once
#include <vector>
#include <memory>
#include <optional>
#include "../bytecode/BytecodeProgram.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include "context/ExecutionContext.hpp"
#include "stack/StackManager.hpp"

// Forward declarations for JIT
namespace vm::jit {
    class JitProfiler;
    class JitCodeCache;
    class JitCompiler;
    class OSRManager;
}

// Forward declarations for inline caching
namespace vm::jit::ic {
    class InlineCacheTable;
    class TypeFeedbackCollector;
}

// Forward declarations of executors
namespace vm::runtime {
    class StackOperationsExecutor;
    class ComparisonExecutor;
    class LogicalExecutor;
    class ArithmeticExecutor;
    class BitwiseExecutor;
    class ControlFlowExecutor;
    class VariableExecutor;
    class FunctionExecutor;
    class TypeExecutor;
    class ArrayExecutor;
    class ObjectExecutor;
    class LambdaExecutor;
    class ExceptionExecutor;
    class PrimitiveMethodExecutor;  // Phase 3
    class InlineCacheExecutor;      // Phase 6
}

// Forward declarations of utility helpers
namespace vm::runtime::utils {
    class ExceptionHandler;
}

// Forward declaration for event loop
namespace runtime {
    class EventLoop;
}

// Forward declaration for exceptions
namespace errors {
    class UserException;
}

namespace vm::runtime
{
    /**
     * Stack-based virtual machine for executing bytecode
     * Provides high-performance execution of compiled mType programs
     * Coordinates specialized instruction executors following SOLID principles
     */
    class VirtualMachine : public std::enable_shared_from_this<VirtualMachine>
    {
    public:
        // State save/restore for async suspension
        struct VMState {
            size_t instructionPointer;
            std::vector<value::Value> stack;
            std::vector<CallFrame> callStack;
        };

    private:
        // Program data
        const bytecode::BytecodeProgram* program;

        // Execution state
        std::shared_ptr<StackManager> stackManager;
        std::vector<CallFrame> callStack;
        size_t maxCallStackSize;  // Maximum allowed call stack depth
        size_t instructionPointer;

        // Environment integration
        std::shared_ptr<environment::Environment> environment;

        // Event loop integration (for async/await support)
        std::unique_ptr<::runtime::EventLoop> eventLoop;
        size_t currentTaskId;
        std::optional<VMState> savedState;  // For async resumption
        bool suspendedByAwait;  // Flag to indicate suspension by AWAIT instruction

        // Pending rejection from an awaited promise (set by catch_ callback, thrown on resume)
        struct PendingAwaitRejection {
            value::Value exceptionValue;
            std::string exceptionTypeName;
            std::string errorMessage;
        };
        std::optional<PendingAwaitRejection> pendingAwaitRejection;

        // Execution statistics
        ExecutionStats stats;
        std::chrono::steady_clock::time_point executionStart;

        // Debugging state
        bool debuggingEnabled;
        std::string currentSourceFile;
        int currentSourceLine;

        // Exception handling state
        size_t currentFinallyOffset;  // Offset of the currently executing finally block (SIZE_MAX if not in finally)
        std::unique_ptr<errors::UserException> pendingException;  // Exception waiting to be re-thrown after finally block
        size_t pendingFinallyOffset;  // Offset of the finally block that has a pending exception (SIZE_MAX if none)

        // Execution context (persists across calls so executors don't hold dangling refs)
        std::unique_ptr<ExecutionContext> executionCtx;
        void ensureExecutors();

        // Specialized executors
        std::unique_ptr<StackOperationsExecutor> stackOpsExecutor;
        std::unique_ptr<ComparisonExecutor> comparisonExecutor;
        std::unique_ptr<LogicalExecutor> logicalExecutor;
        std::unique_ptr<ArithmeticExecutor> arithmeticExecutor;
        std::unique_ptr<BitwiseExecutor> bitwiseExecutor;
        std::unique_ptr<ControlFlowExecutor> controlFlowExecutor;
        std::unique_ptr<VariableExecutor> variableExecutor;
        std::unique_ptr<FunctionExecutor> functionExecutor;
        std::unique_ptr<TypeExecutor> typeExecutor;
        std::unique_ptr<ArrayExecutor> arrayExecutor;
        std::unique_ptr<ObjectExecutor> objectExecutor;
        std::unique_ptr<LambdaExecutor> lambdaExecutor;
        std::unique_ptr<ExceptionExecutor> exceptionExecutor;
        std::unique_ptr<PrimitiveMethodExecutor> primitiveMethodExecutor;  // Phase 3

        // Utility helpers
        std::unique_ptr<utils::ExceptionHandler> exceptionHandler;

        // JIT compilation support
        std::unique_ptr<vm::jit::JitProfiler> jitProfiler;
        std::unique_ptr<vm::jit::JitCodeCache> jitCodeCache;
        std::unique_ptr<vm::jit::JitCompiler> jitCompiler;
        std::unique_ptr<vm::jit::OSRManager> osrManager;
        bool jitEnabled;

        // Phase 6: Inline caching and type specialization
        std::unique_ptr<vm::jit::ic::InlineCacheTable> inlineCacheTable;
        std::unique_ptr<vm::jit::ic::TypeFeedbackCollector> typeFeedbackCollector;
        std::unique_ptr<InlineCacheExecutor> inlineCacheExecutor;
        bool icEnabled;

    public:
        explicit VirtualMachine(std::shared_ptr<environment::Environment> env,
                               size_t maxStackDepth = 0);  // 0 means use default from constants
        ~VirtualMachine(); // Must be declared in .cpp where executor types are complete

        // Execution
        value::Value execute(const bytecode::BytecodeProgram& bytecodeProgram);
        value::Value executeFunction(const std::string& functionName, const std::vector<value::Value>& args);

        // C++ Interop API - Object creation and method invocation
        value::Value createObject(const std::string& className, const std::vector<value::Value>& args);
        value::Value invokeMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                 const std::string& methodName,
                                 const std::vector<value::Value>& args);
        value::Value invokeStaticMethod(const std::string& className,
                                        const std::string& methodName,
                                        const std::vector<value::Value>& args);
        value::Value invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                  const std::vector<value::Value>& args);

        // C++ Interop API - Field access
        value::Value getField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                             const std::string& fieldName);
        void setField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                     const std::string& fieldName,
                     const value::Value& value);
        value::Value getStaticField(const std::string& className, const std::string& fieldName);
        void setStaticField(const std::string& className, const std::string& fieldName, const value::Value& value);

        // Program management
        void setProgram(const bytecode::BytecodeProgram* prog) {
            program = prog;
            if (executionCtx) { executionCtx->program = prog; }
        }

        // Event loop integration (lazy initialization)
        ::runtime::EventLoop* getEventLoop() const { return eventLoop.get(); }
        ::runtime::EventLoop* ensureEventLoop();  // Create EventLoop if it doesn't exist
        void setCurrentTaskId(size_t taskId) { currentTaskId = taskId; }

        // State save/restore for async suspension
        VMState saveState() const;
        void restoreState(const VMState& state);
        bool hasSavedState() const { return savedState.has_value(); }
        void clearSavedState() { savedState.reset(); }

        // State inspection
        const ExecutionStats& getStats() const;
        std::vector<std::string> getStackTrace() const;

        // Debugging support
        void setDebuggingEnabled(bool enabled) { debuggingEnabled = enabled; }
        bool isDebuggingEnabled() const { return debuggingEnabled; }
        std::shared_ptr<environment::Environment> getEnvironment() const { return environment; }
        const std::vector<CallFrame>& getCallStack() const { return callStack; }
        std::shared_ptr<StackManager> getStackManager() const { return stackManager; }
        const bytecode::BytecodeProgram* getProgram() const { return program; }

        // JIT compilation control
        void setJitEnabled(bool enabled);
        bool isJitEnabled() const { return jitEnabled; }
        vm::jit::JitCodeCache* getJitCodeCache() const { return jitCodeCache.get(); }
        vm::jit::JitProfiler* getJitProfiler() const { return jitProfiler.get(); }
        vm::jit::JitCompiler* getJitCompiler() const { return jitCompiler.get(); }
        vm::jit::OSRManager* getOSRManager() const { return osrManager.get(); }
        void printJitStats() const;
        vm::jit::ic::InlineCacheTable* getInlineCacheTable() const { return inlineCacheTable.get(); }
        vm::jit::ic::TypeFeedbackCollector* getTypeFeedbackCollector() const { return typeFeedbackCollector.get(); }

        // Phase 6: Inline caching control
        void setICEnabled(bool enabled);
        bool isICEnabled() const { return icEnabled; }

        // JIT helper: execute a function call from JIT code via interpreter
        value::Value callFunctionFromJit(const std::string& funcName, const std::vector<value::Value>& args);

        // JIT helper: execute a method call from JIT code via interpreter (avoids re-entrant interpretLoop)
        value::Value callMethodFromJit(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                       const std::string& methodName,
                                       const std::vector<value::Value>& args);

        // Reset VM state
        void reset();

        // GC: Collect root set for garbage collection
        std::vector<void*> collectGCRoots() const;

    private:
        // Main execution loop
        value::Value interpretLoop();

        // Instruction dispatch
        void executeInstruction(const bytecode::BytecodeProgram::Instruction& instr);

        // Extracted dispatch helpers (reduce executeInstruction size)
        void trySpecializeArithmetic(const bytecode::BytecodeProgram::Instruction& instr,
                                     bytecode::OpCode specializedOpcode);
        void executeCallWithJit(const bytecode::BytecodeProgram::Instruction& instr);
        void executeAwait();

        // Helper methods (will be moved to utility classes)
        bool isTruthy(const value::Value& val) const;
        std::string valueToString(const value::Value& val) const;
        value::Value performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op);
        void checkStackUnderflow(size_t required) const;
        value::ValueType stringToValueType(const std::string& typeName);
        std::shared_ptr<value::NativeArray> createJaggedArray(const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName, size_t totalDimensions);
        std::shared_ptr<value::NativeArray> createNestedArray(const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName);

        // Stack operation helpers (temporary until moved to StackManager)
        void push(const value::Value& value);
        value::Value pop();
        value::Value peek(size_t offset = 0) const;
        void popN(size_t count);

        // Call stack management with overflow protection
        void pushCallFrame(const CallFrame& frame);
    };
}
