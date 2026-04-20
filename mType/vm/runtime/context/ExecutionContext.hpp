#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include "../../../value/ValueType.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../stack/StackManager.hpp"

// Forward declaration for VM back-pointer
namespace vm::runtime { class VirtualMachine; }

namespace vm::runtime
{

    /**
     * Shared stack frame for late-bound variable access in lambdas
     * This allows multiple lambdas to share and access the same local variable space
     * Supports parent frame references for nested closures
     */
    struct SharedStackFrame {
        std::vector<value::Value> locals;  // Local variables in this frame
        std::unordered_map<std::string, size_t> nameToSlot;  // Map variable names to slots
        std::shared_ptr<SharedStackFrame> parentFrame;  // Parent frame for nested closures

        value::Value getLocal(size_t slot) const {
            if (slot < locals.size()) {
                return locals[slot];
            }
            return std::monostate{};  // sentinel for slot not found
        }

        value::Value getLocalByName(const std::string& name) const {
            // First check this frame
            auto it = nameToSlot.find(name);
            if (it != nameToSlot.end()) {
                return getLocal(it->second);
            }
            // If not found, check parent frame
            if (parentFrame) {
                return parentFrame->getLocalByName(name);
            }
            return std::monostate{};  // sentinel for not found
        }

        void setLocal(size_t slot, const value::Value& value) {
            if (slot >= locals.size()) {
                locals.resize(slot + 1, std::monostate{});  // sentinel for uninitialized slots
            }
            locals[slot] = value;
        }

        void setLocal(const std::string& name, size_t slot, const value::Value& value) {
            nameToSlot[name] = slot;
            setLocal(slot, value);
        }

        void setLocalByName(const std::string& name, const value::Value& value) {
            // First check this frame
            auto it = nameToSlot.find(name);
            if (it != nameToSlot.end()) {
                setLocal(it->second, value);
                return;
            }
            // If not found in this frame, check parent frame
            if (parentFrame) {
                parentFrame->setLocalByName(name, value);
            }
        }
    };

    /**
     * Bytecode lambda representation with closure support
     * Uses reference capture - captured variables are accessed from shared frame at invocation time
     */
    struct BytecodeLambda {
        size_t instructionPointer;  // Where the lambda code starts
        size_t parameterCount;      // Number of parameters
        std::shared_ptr<SharedStackFrame> capturedFrame;  // Shared frame containing captured variables
        std::vector<size_t> capturedSlots;  // Slot indices in the shared frame to capture
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> capturedThis;  // Captured 'this' from method context
        std::string creatingClassName;  // Class context where lambda was created (for access checks)
        std::vector<std::string> parameterNames;  // Names of lambda parameters (for debugging)
        std::vector<std::string> capturedNames;  // Names of captured variables (for debugging)
        // MYT-197: interned handle into the owning program's frame-name table.
        // Set once at lambda creation (LambdaExecutor LAMBDA handler) and
        // copied as 4 bytes per invocation instead of per-invocation
        // std::string copy.
        vm::bytecode::FunctionNameHandle functionName{ vm::bytecode::INVALID_FN_HANDLE };
    };

    /**
     * Call frame for function invocation
     */
    struct CallFrame {
        size_t returnAddress;                    // Where to return after function completes
        size_t frameBase;                        // Base pointer in operand stack
        size_t localBase;                        // Base of local variables
        // MYT-197: interned handle into the owning program's frame-name table.
        // Resolve via program->getFrameName(handle) for formatting; use
        // program->getFunctionMeta(handle) for O(1) metadata lookup.
        vm::bytecode::FunctionNameHandle functionName{ vm::bytecode::INVALID_FN_HANDLE };
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> thisInstance;  // For method calls
        std::shared_ptr<BytecodeLambda> originatingLambda;  // If this frame is for a lambda, reference to it (for debugging)
        std::string definingClassName;           // Class that defines the method (for access control in inheritance)
        std::shared_ptr<SharedStackFrame> sharedFrame;  // Shared frame for closure capture (if this function creates lambdas)
        size_t programIndex = 0;                 // Which program in loadedPrograms this frame belongs to
    };

    /**
     * Execution statistics for profiling
     */
    struct ExecutionStats {
        size_t instructionsExecuted = 0;
        size_t functionCalls = 0;
        size_t memoryAllocated = 0;
        std::chrono::microseconds executionTime{0};
    };

    /**
     * Shared execution context passed to all instruction executors
     * Provides access to VM state and shared resources
     */
    class ExecutionContext
    {
    public:
        // Program data
        const bytecode::BytecodeProgram* program;

        // Multi-program support: loaded library programs (index 0 = main)
        std::vector<const bytecode::BytecodeProgram*>* loadedPrograms = nullptr;

        // Execution state
        size_t& instructionPointer;
        std::vector<CallFrame>& callStack;
        size_t maxCallStackSize;  // Maximum allowed call stack depth

        // Environment integration
        std::shared_ptr<environment::Environment> environment;

        // Stack manager
        std::shared_ptr<StackManager> stackManager;

        // Execution statistics
        ExecutionStats& stats;
        std::chrono::steady_clock::time_point& executionStart;

        // Debugging state
        bool& debuggingEnabled;
        std::string& currentSourceFile;
        int& currentSourceLine;

        // VM back-pointer for OSR and other VM-level operations
        VirtualMachine* vm = nullptr;

        ExecutionContext(
            const bytecode::BytecodeProgram* prog,
            size_t& ip,
            std::vector<CallFrame>& cs,
            size_t maxStackDepth,
            std::shared_ptr<environment::Environment> env,
            std::shared_ptr<StackManager> sm,
            ExecutionStats& st,
            std::chrono::steady_clock::time_point& exStart,
            bool& debugEnabled,
            std::string& srcFile,
            int& srcLine,
            VirtualMachine* vmPtr = nullptr)
            : program(prog)
            , instructionPointer(ip)
            , callStack(cs)
            , maxCallStackSize(maxStackDepth)
            , environment(std::move(env))
            , stackManager(std::move(sm))
            , stats(st)
            , executionStart(exStart)
            , debuggingEnabled(debugEnabled)
            , currentSourceFile(srcFile)
            , currentSourceLine(srcLine)
            , vm(vmPtr)
        {}

        // Call stack management with overflow protection
        void pushCallFrame(const CallFrame& frame);

        // MYT-173: obtain a mutable reference to the instruction at `offset` in
        // the current program. Runtime opcode rewriting (CALL_METHOD <->
        // CALL_METHOD_CACHED and analogous future specializations) legitimately
        // needs to mutate the `Instruction::opcode` field. `program` is typed as
        // `const BytecodeProgram*` by convention (read-mostly), but the pointee
        // is never declared `const` at the object level — every
        // `BytecodeProgram` in the VM is created as a non-const object and owned
        // non-const by the VM/loadedPrograms storage. Centralising the cast here
        // keeps that invariant documented in exactly one place; callers stay
        // agnostic.
        bytecode::BytecodeProgram::Instruction& getMutableInstructionAt(size_t offset) const {
            return const_cast<bytecode::BytecodeProgram*>(program)
                ->getMutableInstruction(offset);
        }

        // MYT-197: resolve the BytecodeProgram that owns a given frame's
        // FunctionNameHandle. Use this when formatting a frame that isn't
        // necessarily on the currently-executing program (stack-trace walks,
        // exception unwinding, debugger frame inspection). Falls back to
        // `program` when loadedPrograms isn't wired or programIndex is out
        // of range.
        const bytecode::BytecodeProgram* programForFrame(const CallFrame& frame) const {
            if (loadedPrograms && frame.programIndex < loadedPrograms->size()) {
                return (*loadedPrograms)[frame.programIndex];
            }
            return program;
        }

        const std::string& frameName(const CallFrame& frame) const {
            return programForFrame(frame)->getFrameName(frame.functionName);
        }
    };
}
