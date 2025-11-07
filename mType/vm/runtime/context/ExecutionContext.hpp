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
            return std::monostate{};
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
            return std::monostate{};
        }

        void setLocal(size_t slot, const value::Value& value) {
            if (slot >= locals.size()) {
                locals.resize(slot + 1, std::monostate{});
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
        std::string functionName;  // Unique lambda function name (for metadata lookup)
    };

    /**
     * Call frame for function invocation
     */
    struct CallFrame {
        size_t returnAddress;                    // Where to return after function completes
        size_t frameBase;                        // Base pointer in operand stack
        size_t localBase;                        // Base of local variables
        std::string functionName;                // For debugging/stack traces
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> thisInstance;  // For method calls
        std::shared_ptr<BytecodeLambda> originatingLambda;  // If this frame is for a lambda, reference to it (for debugging)
        std::string definingClassName;           // Class that defines the method (for access control in inheritance)
        std::shared_ptr<SharedStackFrame> sharedFrame;  // Shared frame for closure capture (if this function creates lambdas)
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
            int& srcLine)
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
        {}

        // Call stack management with overflow protection
        void pushCallFrame(const CallFrame& frame);
    };
}
