#pragma once
#include <vector>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <algorithm>
#include <span>
#include <unordered_set>
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

// Forward declaration for async promise (MYT-113 interop)
namespace value {
    class AsyncPromiseValue;
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

        // MYT-197: pre-warmed handle for the "__script_main__" sentinel. Set
        // in execute() at program bind time so the top-of-interpret check
        // `callStack.back().functionName == scriptMainHandle` is an integer
        // compare instead of a hashmap-probe per pop.
        bytecode::FunctionNameHandle scriptMainHandle{ bytecode::INVALID_FN_HANDLE };

        // Multi-program support: loaded library programs (index 0 = main)
        std::vector<const bytecode::BytecodeProgram*> loadedPrograms;
        std::unordered_set<const bytecode::BytecodeProgram*> staticInitializedPrograms;
        // MYT-325: per-name dedup so the same `<ClassName>::<static_init>$static`
        // doesn't run twice when a class lives in both the embedded main
        // bytecode and a sidecar .mtcLib (or any other multi-program scenario).
        std::unordered_set<std::string> executedStaticInitializers;

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

        // MYT-113: Async-interop mode. When true, executeAwait stores the
        // awaited promise here and sets suspendedByAwait WITHOUT registering
        // event-loop callbacks or calling suspendCurrentTask. invokeMethod /
        // invokeStaticMethod / invokeLambda use this to drive an async body
        // through nested awaits via continuations chained on the awaited
        // promise, settling an outer Promise<T> only when the body completes.
        bool inInteropAsyncMode = false;
        std::shared_ptr<value::AsyncPromiseValue> interopAwaitedPromise;
        // Rejection from a previous interop suspension's catch_ continuation,
        // consumed at the start of the next driveAsyncInvocation call.
        // Kept separate from pendingAwaitRejection (which the outer loop
        // uses) so the two can't clobber each other.
        std::optional<PendingAwaitRejection> interopPendingRejection;

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
        size_t jitNativeDepth = 0;  // Tracks JIT native recursion depth to prevent C++ stack overflow

        // Cycle-prevention flag: set while runJitMiniInterpret is draining a
        // frame on behalf of a JIT-helper fallback (callFunctionFromJit /
        // callMethodFromJit). When set, executeCallWithJit / _Fast skip JIT
        // dispatch and route directly to the interpreter's handleCall —
        // avoiding the 8-native-frame-per-iteration cycle that otherwise
        // forms when a JIT-fallback mini-interpret hits a CALL whose JIT
        // dispatch re-spawns another callFunctionFromJit (the asmjit-frame
        // sizes blow Windows' 1MB native stack before the managed callStack's
        // own 1000-frame overflow check fires). Non-recursive calls inside
        // the fallback also stay in the interpreter — minor speed loss in a
        // path that was already pessimised by the helper bailing out.
        bool inJitFallbackInterpreter = false;

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
        void runStaticInitializers(const bytecode::BytecodeProgram& bytecodeProgram);
        void runStaticInitializersForLoadedPrograms();

        // C++ Interop API - Object creation and method invocation.
        // MYT-208: takes a span so JIT helpers (jit_new_object, jit_new_stack)
        // pass `callArgs` + count without heap-allocating a per-call vector.
        // External callers passing std::vector<Value> work via std::span's
        // implicit construction from a contiguous range.
        value::Value createObject(const std::string& className, std::span<const value::Value> args);

        // MYT-208: JIT-side stack-promoted allocation. Mirrors createObject but
        // calls ObjectInstancePool::acquireRaw, pushes the raw pointer onto
        // the current frame's stackObjects, and returns a STACK_OBJECT-tagged
        // Value. Non-trivial ctors fall back to createObject (heap) to keep
        // the v1 implementation small — the dominant `class { int F = a }`
        // pattern hits the trivial-ctor fast path here.
        value::Value createStackObject(const std::string& className, std::span<const value::Value> args);

    private:
        // MYT-113: Drive an async method/lambda body that may suspend on awaits.
        // Called after the inner frame has been pushed and instructionPointer
        // is set to the body start. Runs the inner loop in interop async mode;
        // on suspension, registers a continuation on the awaited promise that
        // restores the inner frame state and re-enters this driver, eventually
        // settling `outerPromise` when the body fully completes (or throws).
        // Returns immediately after the first suspension or upon completion.
        void driveAsyncInvocation(
            std::shared_ptr<value::AsyncPromiseValue> outerPromise,
            size_t outerSavedIP,
            std::vector<CallFrame> outerSavedCallStack,
            size_t outerSavedFinally,
            size_t frameBase);

    public:
        value::Value invokeMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                 const std::string& methodName,
                                 std::span<const value::Value> args);
        value::Value invokeStaticMethod(const std::string& className,
                                        const std::string& methodName,
                                        std::span<const value::Value> args);
        value::Value invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                  const std::vector<value::Value>& args);
        value::Value invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                  const value::Value* args,
                                  size_t argCount);

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
            // Only forget the OLD program's init state on swap. Library
            // entries in staticInitializedPrograms must survive a main-
            // program swap, otherwise loaded libraries get re-initialized
            // and any side effects (counter increments, registrations)
            // run twice.
            if (program != prog && program != nullptr) {
                staticInitializedPrograms.erase(program);
            }
            program = prog;
            if (executionCtx) { executionCtx->program = prog; }
            // Ensure main program is at index 0 in loadedPrograms
            if (loadedPrograms.empty()) {
                loadedPrograms.push_back(prog);
            } else {
                loadedPrograms[0] = prog;
            }
        }

        // Multi-program support (library loading)
        void addLoadedProgram(const bytecode::BytecodeProgram* prog) {
            loadedPrograms.push_back(prog);
        }
        [[nodiscard]] bool removeLoadedProgram(const bytecode::BytecodeProgram* prog) {
            // Never remove the main program
            if (prog == program) {
                return false;
            }
            auto it = std::find(loadedPrograms.begin(), loadedPrograms.end(), prog);
            if (it != loadedPrograms.end()) {
                loadedPrograms.erase(it);
                return true;
            }
            return false;
        }
        size_t getLoadedProgramCount() const { return loadedPrograms.size(); }
        const std::vector<const bytecode::BytecodeProgram*>& getLoadedPrograms() const {
            return loadedPrograms;
        }

        // MYT-197: resolve the BytecodeProgram that owns a given frame's
        // FunctionNameHandle. Use when formatting a frame that may not be
        // on the currently-executing program (stack-trace walks, exception
        // unwinding, debugger frame inspection). Falls back to the main
        // program when programIndex is out of range.
        const bytecode::BytecodeProgram* programForFrame(const CallFrame& frame) const {
            if (frame.programIndex < loadedPrograms.size()) {
                return loadedPrograms[frame.programIndex];
            }
            return program;
        }

        const std::string& frameName(const CallFrame& frame) const {
            return programForFrame(frame)->getFrameName(frame.functionName);
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
        size_t getInstructionPointer() const { return instructionPointer; }
        // MYT-228: mutable access for JIT helpers that need to consult the
        // current top frame's typeArgBindings (jit_cast_typeparam,
        // jit_instanceof_typeparam, jit_bind_type_args).
        std::vector<CallFrame>& getCallStackMutable() { return callStack; }
        // MYT-228: write to ExecutionContext::pendingTypeArgs from JIT
        // helpers. The very next pushCallFrame consumes the slot. Defined
        // out-of-line in VirtualMachine.cpp to avoid pulling
        // ExecutionContext.hpp here for the inline.
        void setPendingTypeArgs(std::unordered_map<std::string, std::string> bindings);

        // MYT-228: acquire a cleared pool-backed map for the JIT helper to
        // populate in-place — avoids the local-map-then-move dance the
        // setPendingTypeArgs path costs. Returns a reference to the
        // ExecutionContext's pending slot's map. Caller must ensure
        // ExecutionContext has been initialized.
        TypeArgMap& beginPendingTypeArgs();
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

    private:
        // Per-category sections of printJitStats. Split out for readability;
        // each prints one labeled block.
        void printJitFunctionProfilingStats() const;
        void printJitCompilationStats() const;
        void printJitFieldIcStats() const;
        void printJitInlineDecisionStats() const;
        void printJitLoopOsrStats() const;
    public:
        vm::jit::ic::InlineCacheTable* getInlineCacheTable() const { return inlineCacheTable.get(); }
        vm::jit::ic::TypeFeedbackCollector* getTypeFeedbackCollector() const { return typeFeedbackCollector.get(); }

        // MYT-316: function-redefinition / plugin-reload invalidation. When a
        // function's body is replaced, every JIT'd caller that speculatively
        // pasted the old body inline must be evicted — there is no runtime
        // identity guard. Looks up reverse caller→callee edges in
        // JitCodeCache (registerInlineEdge populates them during emit),
        // invalidates each caller's JIT buffer, and scrubs the IC table to
        // satisfy MYT-315's cachedJit-pointer contract. No-op when no caller
        // ever inlined `callee`. Modeled on
        // BytecodeProgram::clearNativeCacheSlots — eager eviction, no
        // runtime check.
        void invalidateInlinedFunctionCallers(bytecode::FunctionNameHandle callee);

        // Phase 6: Inline caching control
        void setICEnabled(bool enabled);
        bool isICEnabled() const { return icEnabled; }

        // JIT helper: execute a function call from JIT code via interpreter
        value::Value callFunctionFromJit(const std::string& funcName, std::span<const value::Value> args);

        // JIT IC fast path: skip the program->getFunction(funcName) hashmap
        // lookup when the caller already has the resolved FunctionMetadata
        // cached at the call-site IP.
        value::Value callFunctionFromJitDirect(const std::string& funcName,
                                                const bytecode::BytecodeProgram::FunctionMetadata* funcMeta,
                                                std::span<const value::Value> args);

    private:
        // Shared mini-interpret loop used by callFunctionFromJit and the two
        // callMethodFromJitDirect overloads. The caller is responsible for
        // pushing args/locals, pushing the call frame, and setting
        // instructionPointer to the function's startOffset. The helper
        // increments jitNativeDepth, runs instructions until the call frame
        // pops back to savedCallStackDepth, pops the return value, restores
        // IP (and the saved program if switchedProgram), and returns it. On
        // any exception it unwinds intermediate frames, restores stack/IP/
        // program, decrements jitNativeDepth, and re-throws the original
        // exception object intact (the throw; preserves type and what()).
        value::Value runJitMiniInterpret(size_t savedIP,
                                          size_t savedCallStackDepth,
                                          size_t savedStackSize,
                                          const bytecode::BytecodeProgram* savedProgram,
                                          bool switchedProgram);

        // Shared exception-unwind for runJitMiniInterpret: pops back to
        // savedCallStackDepth releasing stack-promoted allocations, restores
        // instruction pointer, drains the operand stack to savedStackSize,
        // and restores executionCtx->program if it was switched.
        void restoreJitMiniInterpretState(size_t savedIP,
                                           size_t savedCallStackDepth,
                                           size_t savedStackSize,
                                           const bytecode::BytecodeProgram* savedProgram,
                                           bool switchedProgram);

        // MYT-254 in-scope UserException dispatch path. Returns true if the
        // exception was dispatched to a covering handler in the current frame
        // (caller should `continue` the loop); false if the exception must
        // unwind past the mini-interpret frame.
        bool tryDispatchInScopeJitUserException(
            errors::UserException& e,
            size_t savedCallStackDepth,
            const bytecode::BytecodeProgram* jitCurrentProgram);

    public:


        // JIT helper: execute a method call from JIT code via interpreter (avoids re-entrant interpretLoop)
        value::Value callMethodFromJit(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                       const std::string& methodName,
                                       const std::vector<value::Value>& args);

        // Value-class overload: dispatches without materialising a temp ObjectInstance.
        // The Value receiver carries either an ObjectInstance bridge or a ValueObject
        // shared_ptr; the method body's GET_FIELD / SET_FIELD / LOAD_LOCAL handlers
        // already accept value-object receivers so the only difference vs the
        // shared_ptr<ObjectInstance> overload is what gets pushed as local-0.
        value::Value callMethodFromJit(const value::Value& receiver,
                                       const std::string& methodName,
                                       const std::vector<value::Value>& args);

        // JIT IC fast-path: skip method resolution by accepting pre-resolved metadata.
        // qualifiedName must include defining class prefix (e.g. "Shape::area"); funcMetadata
        // must point to that function's bytecode metadata. Used by jit_call_method_ic on hit.
        // MYT-182: calleeProgram, when non-null and different from the current
        // executionCtx->program, causes this call to run its mini-interpret
        // loop against the callee's program and restore on exit — required
        // for cross-program (library) dispatch.
        value::Value callMethodFromJitDirect(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                             const std::string& qualifiedName,
                                             const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
                                             const std::vector<value::Value>& args,
                                             const bytecode::BytecodeProgram* calleeProgram = nullptr);

        // Value-class overload — same protocol as the shared_ptr form, but
        // pushes the receiver Value directly as local-0. Required by the
        // unified IC-hit fast-dispatch (Stage B).
        value::Value callMethodFromJitDirect(const value::Value& receiver,
                                             const std::string& qualifiedName,
                                             const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
                                             const std::vector<value::Value>& args,
                                             const bytecode::BytecodeProgram* calleeProgram = nullptr);

        // MYT-208: STACK_OBJECT-receiver direct dispatch. Same body as the
        // shared_ptr overload but threads the raw ObjectInstance* through
        // frame.thisInstanceRaw and pushes the STACK_OBJECT Value (no
        // shared_ptr copy) as local-0. Caller (callMethodFromJitDirect Value
        // overload) verifies the tag before calling.
        value::Value callMethodFromJitDirectStack(const value::Value& receiverValue,
                                                  const std::string& qualifiedName,
                                                  const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
                                                  const std::vector<value::Value>& args,
                                                  const bytecode::BytecodeProgram* calleeProgram = nullptr);

        // JIT helper (MYT-146): allocate a multi-dimensional array. Mirrors
        // ArrayExecutor::handleNewArrayMulti's post-pop dispatch but takes
        // pre-popped dimensions so it's callable without an ExecutionContext.
        value::Value createMultiArrayFromJit(uint32_t typeNameIndex,
                                             const std::vector<int64_t>& dimensions,
                                             size_t totalDimensions);

        // Reset VM state
        void reset();

        // GC: Collect root set for garbage collection
        std::vector<void*> collectGCRoots() const;

    private:
        // Main execution loop
        value::Value interpretLoop();

        // Debug hook shared by interpretLoop and the interop invoke paths
        // (invokeMethod/invokeStaticMethod/invokeLambda/invokeConstructor/
        // driveAsyncInvocation). isDebugActive() folds the per-VM flag with the
        // global debugger state; debugPauseIfNeeded() performs the breakpoint
        // check + waitForResume at the current instructionPointer. Defined in
        // VirtualMachineLoop.cpp so the debug-helper includes stay confined to
        // one TU. NOT used on the JIT mini-interpreter path — JIT and the
        // debugger are mutually exclusive and adding work to that hot loop
        // perturbs JIT/OSR heuristics. (VK-1378)
        bool isDebugActive() const;
        void debugPauseIfNeeded();

        // Instruction dispatch
        void executeInstruction(const bytecode::BytecodeProgram::Instruction& instr);

        // Split-file dispatch fall-throughs. Primary executeInstruction falls
        // through to dispatchExtendedOps for object/iterator/primitive/array
        // opcodes, and from there to dispatchSpecialOps for type/lambda/
        // exception/async/debug/profile opcodes. Splitting the giant switch
        // across TUs keeps each .cpp under the 500-line cap.
        void dispatchExtendedOps(const bytecode::BytecodeProgram::Instruction& instr);
        void dispatchSpecialOps(const bytecode::BytecodeProgram::Instruction& instr);

        // Extracted dispatch helpers (reduce executeInstruction size)
        void trySpecializeArithmetic(const bytecode::BytecodeProgram::Instruction& instr,
                                     bytecode::OpCode intOpcode, bytecode::OpCode floatOpcode);
        // Bitwise-op runtime promotion. Mirrors trySpecializeArithmetic but
        // only INT is valid — bitwise on float is a type error, so there's no
        // FLOAT variant to pick. Callers pass the binary or unary INT opcode
        // to promote to after observing monomorphic-INT operands via type
        // feedback.
        void trySpecializeBitwise(const bytecode::BytecodeProgram::Instruction& instr,
                                  bytecode::OpCode intOpcode);
        void trySpecializeBitwiseUnary(const bytecode::BytecodeProgram::Instruction& instr,
                                       bytecode::OpCode intOpcode);
        // MYT-198: runtime fusion of PUSH_INT + ADD_INT into ADD_INT_CONST.
        // Called from trySpecializeArithmetic immediately after the ADD →
        // ADD_INT promotion lands. Keeps the symmetry with the _CACHED
        // fusions driven by InlineCacheExecutor::tryFuseWithPriorLoadLocal.
        void tryFuseAddIntConst();
        void executeCallWithJit(const bytecode::BytecodeProgram::Instruction& instr);
        void executeCallFastWithJit(const bytecode::BytecodeProgram::Instruction& instr);
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

    public:
        // Call stack management with overflow protection (public for JIT access).
        // Takes the frame by value — CallFrame is move-only (TypeArgMapPtr
        // owns a pool slot). All call sites use `pushCallFrame(std::move(frame))`.
        void pushCallFrame(CallFrame frame);
        void popCallStack();

        // JIT native depth tracking (public for JIT helpers access)
        static constexpr size_t MAX_JIT_NATIVE_DEPTH = 256;
        size_t getJitNativeDepth() const { return jitNativeDepth; }
        void incrementJitNativeDepth() { ++jitNativeDepth; }
        void decrementJitNativeDepth() { --jitNativeDepth; }

        // MYT-207: byte offset of jitNativeDepth so JIT-emitted code can
        // inline the depth-guard cmp + inc/dec without paying a cc.invoke
        // round-trip per self-recursive call. Defined out-of-line below so
        // the offsetof macro sees a complete type (MSVC's strict offsetof
        // refuses incomplete-type uses inside the class body). Read by
        // tryEmitSelfDirectCall.
        static const size_t kJitNativeDepthOffset;
        static const size_t kMaxCallStackSizeOffset;
    };

    // MYT-207: out-of-line definition. `inline` lets the same constant live
    // in every TU that includes this header without ODR conflicts. Must be
    // outside the class body so the type is complete for offsetof, and must
    // be inside vm::runtime:: so jitNativeDepth (private) is reachable
    // through the VirtualMachine:: qualified name.
    // These offsets are part of the JIT ABI: generated code reads/writes
    // the fields directly through the VM pointer. VirtualMachine is not a
    // standard-layout type, so Clang warns on offsetof even though this is
    // the intended compiler-supported extension here.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    inline const size_t VirtualMachine::kJitNativeDepthOffset =
        offsetof(VirtualMachine, jitNativeDepth);
    inline const size_t VirtualMachine::kMaxCallStackSizeOffset =
        offsetof(VirtualMachine, maxCallStackSize);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
