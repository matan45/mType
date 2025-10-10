#pragma once
#include <vector>
#include <memory>
#include <optional>
#include "../bytecode/BytecodeProgram.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include "context/ExecutionContext.hpp"
#include "stack/StackManager.hpp"

// Forward declarations of executors
namespace vm::runtime {
    class StackOperationsExecutor;
    class ComparisonExecutor;
    class LogicalExecutor;
    class ArithmeticExecutor;
    class ControlFlowExecutor;
    class VariableExecutor;
    class FunctionExecutor;
    class TypeExecutor;
    class ArrayExecutor;
    class ObjectExecutor;
    class LambdaExecutor;
    class ExceptionExecutor;
}

// Forward declaration for event loop
namespace runtime {
    class EventLoop;
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
        size_t instructionPointer;

        // Environment integration
        std::shared_ptr<environment::Environment> environment;

        // Event loop integration (for async/await support)
        std::unique_ptr<::runtime::EventLoop> eventLoop;
        size_t currentTaskId;
        std::optional<VMState> savedState;  // For async resumption
        bool suspendedByAwait;  // Flag to indicate suspension by AWAIT instruction

        // Execution statistics
        ExecutionStats stats;
        std::chrono::steady_clock::time_point executionStart;

        // Exception handling state
        size_t currentFinallyOffset;  // Offset of the currently executing finally block (SIZE_MAX if not in finally)

        // Specialized executors
        std::unique_ptr<StackOperationsExecutor> stackOpsExecutor;
        std::unique_ptr<ComparisonExecutor> comparisonExecutor;
        std::unique_ptr<LogicalExecutor> logicalExecutor;
        std::unique_ptr<ArithmeticExecutor> arithmeticExecutor;
        std::unique_ptr<ControlFlowExecutor> controlFlowExecutor;
        std::unique_ptr<VariableExecutor> variableExecutor;
        std::unique_ptr<FunctionExecutor> functionExecutor;
        std::unique_ptr<TypeExecutor> typeExecutor;
        std::unique_ptr<ArrayExecutor> arrayExecutor;
        std::unique_ptr<ObjectExecutor> objectExecutor;
        std::unique_ptr<LambdaExecutor> lambdaExecutor;
        std::unique_ptr<ExceptionExecutor> exceptionExecutor;

    public:
        explicit VirtualMachine(std::shared_ptr<environment::Environment> env);
        ~VirtualMachine(); // Must be declared in .cpp where executor types are complete

        // Execution
        value::Value execute(const bytecode::BytecodeProgram& bytecodeProgram);
        value::Value executeFunction(const std::string& functionName, const std::vector<value::Value>& args);

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

        // Reset VM state
        void reset();

    private:
        // Main execution loop
        value::Value interpretLoop();

        // Instruction dispatch
        void executeInstruction(const bytecode::BytecodeProgram::Instruction& instr);

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
    };
}
