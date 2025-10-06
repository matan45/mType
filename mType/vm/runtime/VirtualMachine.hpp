#pragma once
#include <vector>
#include <memory>
#include "../bytecode/BytecodeProgram.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"

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
     * Stack-based virtual machine for executing bytecode
     * Provides high-performance execution of compiled mType programs
     */
    class VirtualMachine
    {
    public:
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

    private:
        // Program data
        const bytecode::BytecodeProgram* program;

        // Execution state
        std::vector<value::Value> operandStack;
        std::vector<CallFrame> callStack;
        size_t instructionPointer;

        // Environment integration
        std::shared_ptr<environment::Environment> environment;

        // Execution statistics
        ExecutionStats stats;
        std::chrono::steady_clock::time_point executionStart;

    public:
        explicit VirtualMachine(std::shared_ptr<environment::Environment> env);
        ~VirtualMachine() = default;

        // Execution
        value::Value execute(const bytecode::BytecodeProgram& bytecodeProgram);
        value::Value executeFunction(const std::string& functionName, const std::vector<value::Value>& args);

        // State inspection
        const ExecutionStats& getStats() const;
        std::vector<std::string> getStackTrace() const;

        // Reset VM state
        void reset();

    private:
        // Main execution loop
        value::Value interpretLoop();

        // Instruction dispatch
        void executeInstruction(const bytecode::BytecodeProgram::Instruction& instr);

        // Stack operations
        void push(const value::Value& value);
        value::Value pop();
        value::Value peek(size_t offset = 0) const;
        void popN(size_t count);

        // Opcode handlers - Organized by category

        // Stack operations
        void handlePushInt(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushString(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushBool(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushNull();
        void handlePop();
        void handleDup();
        void handleSwap();

        // Arithmetic operations
        void handleAdd();
        void handleSub();
        void handleMul();
        void handleDiv();
        void handleMod();
        void handleNeg();
        void handleInc();
        void handleDec();
        void handleAddInt();
        void handleSubInt();
        void handleMulInt();
        void handleDivInt();

        // Comparison operations
        void handleEq();
        void handleNe();
        void handleLt();
        void handleGt();
        void handleLe();
        void handleGe();

        // Logical operations
        void handleAnd();
        void handleOr();
        void handleNot();

        // Variable operations
        void handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr);

        // Control flow operations
        void handleJump(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfFalse(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfTrue(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr);
        void handleReturn();
        void handleReturnValue();

        // Function operations
        void handleCall(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallNative(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr);

        // Object operations
        void handleNewObject(const bytecode::BytecodeProgram::Instruction& instr);
        void handleGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);

        // Array operations
        void handleNewArray(const bytecode::BytecodeProgram::Instruction& instr);
        void handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr);
        void handleArrayGet();
        void handleArraySet();
        void handleArrayLength();

        // Type operations
        void handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCast(const bytecode::BytecodeProgram::Instruction& instr);
        void handleToString();

        // Lambda operations
        void handleLambda(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLambdaInvoke(const bytecode::BytecodeProgram::Instruction& instr);

        // Helper methods
        bool isTruthy(const value::Value& val) const;
        std::string valueToString(const value::Value& val) const;
        value::Value performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op);
        void checkStackUnderflow(size_t required) const;
        value::ValueType stringToValueType(const std::string& typeName);
        std::shared_ptr<value::NativeArray> createJaggedArray(const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName, size_t totalDimensions);
        std::shared_ptr<value::NativeArray> createNestedArray(const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName);
    };
}
