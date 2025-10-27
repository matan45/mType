#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include "../../../value/ValueType.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../stack/StackManager.hpp"

namespace vm::runtime
{

    /**
     * Shared stack frame for late-bound variable access in lambdas
     * This allows multiple lambdas to share and access the same local variable space
     */
    struct SharedStackFrame {
        std::vector<value::Value> locals;  // Local variables in this frame
        std::unordered_map<std::string, size_t> nameToSlot;  // Map variable names to slots

        value::Value getLocal(size_t slot) const {
            if (slot < locals.size()) {
                return locals[slot];
            }
            return std::monostate{};
        }

        value::Value getLocalByName(const std::string& name) const {
            auto it = nameToSlot.find(name);
            if (it != nameToSlot.end()) {
                return getLocal(it->second);
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
    };

    /**
     * Bytecode lambda representation with closure support
     * Uses hybrid capture: snapshot for existing variables, late-binding for forward references
     */
    struct BytecodeLambda {
        size_t instructionPointer;  // Where the lambda code starts
        size_t parameterCount;      // Number of parameters
        std::vector<value::Value> capturedValues;  // Snapshot values of variables at lambda creation time
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> capturedThis;  // Captured 'this' from method context
        std::string creatingClassName;  // Class context where lambda was created (for access checks)
        std::shared_ptr<SharedStackFrame> parentFrame;  // Shared parent frame for late-bound variable access (forward refs)
        std::unordered_map<std::string, size_t> parentVarNames;  // Map of parent variable names to their slots
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
        std::shared_ptr<SharedStackFrame> sharedFrame;  // Shared frame for lambda late-binding
        std::shared_ptr<BytecodeLambda> originatingLambda;  // If this frame is for a lambda, reference to it
        std::string definingClassName;           // NEW: Class that defines the method (for access control in inheritance)
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
            , environment(std::move(env))
            , stackManager(std::move(sm))
            , stats(st)
            , executionStart(exStart)
            , debuggingEnabled(debugEnabled)
            , currentSourceFile(srcFile)
            , currentSourceLine(srcLine)
        {}
    };
}
