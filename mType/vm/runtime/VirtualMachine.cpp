#include "VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include <chrono>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace vm::runtime
{
    VirtualMachine::VirtualMachine(std::shared_ptr<environment::Environment> env)
        : program(nullptr)
        , instructionPointer(0)
        , environment(std::move(env))
    {
        operandStack.reserve(256);  // Reserve initial capacity
        callStack.reserve(64);
    }

    value::Value VirtualMachine::execute(const bytecode::BytecodeProgram& bytecodeProgram) {
        program = &bytecodeProgram;
        instructionPointer = program->getEntryPoint();

        executionStart = std::chrono::steady_clock::now();
        stats = ExecutionStats{};

        try {
            return interpretLoop();
        } catch (...) {
            // Clean up and rethrow
            reset();
            throw;
        }
    }

    value::Value VirtualMachine::executeFunction(const std::string& functionName, const std::vector<value::Value>& args) {
        if (!program) {
            throw errors::RuntimeException("No program loaded");
        }

        auto* funcMetadata = program->getFunction(functionName);
        if (!funcMetadata) {
            throw errors::RuntimeException("Function not found: " + functionName);
        }

        // Push arguments onto stack
        for (const auto& arg : args) {
            push(arg);
        }

        // Set instruction pointer to function start
        instructionPointer = funcMetadata->startOffset;

        return interpretLoop();
    }

    value::Value VirtualMachine::interpretLoop() {
        while (instructionPointer < program->getInstructionCount()) {
            const auto& instr = program->getInstruction(instructionPointer);

            // Execute instruction
            executeInstruction(instr);

            stats.instructionsExecuted++;

            // Check for halt
            if (instr.opcode == bytecode::OpCode::HALT) {
                break;
            }

            instructionPointer++;
        }

        // Calculate execution time
        auto executionEnd = std::chrono::steady_clock::now();
        stats.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(
            executionEnd - executionStart);

        // Return top of stack or void
        if (!operandStack.empty()) {
            return pop();
        }
        return std::monostate{};
    }

    void VirtualMachine::executeInstruction(const bytecode::BytecodeProgram::Instruction& instr) {
        using OpCode = bytecode::OpCode;

        switch (instr.opcode) {
            // Stack operations
            case OpCode::PUSH_INT: handlePushInt(instr); break;
            case OpCode::PUSH_FLOAT: handlePushFloat(instr); break;
            case OpCode::PUSH_STRING: handlePushString(instr); break;
            case OpCode::PUSH_BOOL: handlePushBool(instr); break;
            case OpCode::PUSH_NULL: handlePushNull(); break;
            case OpCode::POP: handlePop(); break;
            case OpCode::DUP: handleDup(); break;
            case OpCode::SWAP: handleSwap(); break;

            // Arithmetic
            case OpCode::ADD: handleAdd(); break;
            case OpCode::SUB: handleSub(); break;
            case OpCode::MUL: handleMul(); break;
            case OpCode::DIV: handleDiv(); break;
            case OpCode::MOD: handleMod(); break;
            case OpCode::NEG: handleNeg(); break;
            case OpCode::ADD_INT: handleAddInt(); break;
            case OpCode::SUB_INT: handleSubInt(); break;
            case OpCode::MUL_INT: handleMulInt(); break;
            case OpCode::DIV_INT: handleDivInt(); break;

            // Comparison
            case OpCode::EQ: handleEq(); break;
            case OpCode::NE: handleNe(); break;
            case OpCode::LT: handleLt(); break;
            case OpCode::GT: handleGt(); break;
            case OpCode::LE: handleLe(); break;
            case OpCode::GE: handleGe(); break;

            // Logical
            case OpCode::AND: handleAnd(); break;
            case OpCode::OR: handleOr(); break;
            case OpCode::NOT: handleNot(); break;

            // Variables
            case OpCode::LOAD_VAR: handleLoadVar(instr); break;
            case OpCode::STORE_VAR: handleStoreVar(instr); break;
            case OpCode::DECLARE_VAR: handleDeclareVar(instr); break;
            case OpCode::LOAD_LOCAL: handleLoadLocal(instr); break;
            case OpCode::STORE_LOCAL: handleStoreLocal(instr); break;

            // Control flow
            case OpCode::JUMP: handleJump(instr); break;
            case OpCode::JUMP_IF_FALSE: handleJumpIfFalse(instr); break;
            case OpCode::JUMP_IF_TRUE: handleJumpIfTrue(instr); break;
            case OpCode::JUMP_BACK: handleJumpBack(instr); break;
            case OpCode::RETURN: handleReturn(); break;
            case OpCode::RETURN_VALUE: handleReturnValue(); break;

            // Functions
            case OpCode::CALL: handleCall(instr); break;
            case OpCode::CALL_NATIVE: handleCallNative(instr); break;

            // Objects
            case OpCode::NEW_OBJECT: handleNewObject(instr); break;
            case OpCode::GET_FIELD: handleGetField(instr); break;
            case OpCode::SET_FIELD: handleSetField(instr); break;
            case OpCode::CALL_METHOD: handleCallMethod(instr); break;

            // Arrays
            case OpCode::NEW_ARRAY: handleNewArray(instr); break;
            case OpCode::ARRAY_GET: handleArrayGet(); break;
            case OpCode::ARRAY_SET: handleArraySet(); break;
            case OpCode::ARRAY_LENGTH: handleArrayLength(); break;

            // Type operations
            case OpCode::INSTANCEOF: handleInstanceof(instr); break;
            case OpCode::CAST: handleCast(instr); break;

            // Special
            case OpCode::HALT:
                // Handled in interpretLoop
                break;

            case OpCode::NOP:
                // Do nothing
                break;

            default:
                throw errors::RuntimeException("Unimplemented opcode: " +
                    std::string(bytecode::getOpCodeName(instr.opcode)));
        }
    }

    // === Stack Operations ===

    void VirtualMachine::push(const value::Value& value) {
        operandStack.push_back(value);
    }

    value::Value VirtualMachine::pop() {
        checkStackUnderflow(1);
        value::Value val = operandStack.back();
        operandStack.pop_back();
        return val;
    }

    value::Value VirtualMachine::peek(size_t offset) const {
        checkStackUnderflow(offset + 1);
        return operandStack[operandStack.size() - 1 - offset];
    }

    void VirtualMachine::popN(size_t count) {
        checkStackUnderflow(count);
        for (size_t i = 0; i < count; ++i) {
            operandStack.pop_back();
        }
    }

    void VirtualMachine::handlePushInt(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_INT requires operand");
        }
        int value = program->getConstantPool().getInteger(instr.operands[0]);
        push(value);
    }

    void VirtualMachine::handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_FLOAT requires operand");
        }
        double value = program->getConstantPool().getFloat(instr.operands[0]);
        push(static_cast<float>(value));
    }

    void VirtualMachine::handlePushString(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_STRING requires operand");
        }
        const std::string& value = program->getConstantPool().getString(instr.operands[0]);
        push(value);
    }

    void VirtualMachine::handlePushBool(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_BOOL requires operand");
        }
        push(instr.operands[0] != 0);
    }

    void VirtualMachine::handlePushNull() {
        push(nullptr);
    }

    void VirtualMachine::handlePop() {
        pop();
    }

    void VirtualMachine::handleDup() {
        push(peek());
    }

    void VirtualMachine::handleSwap() {
        checkStackUnderflow(2);
        value::Value top = pop();
        value::Value second = pop();
        push(top);
        push(second);
    }

    // === Arithmetic Operations ===

    void VirtualMachine::handleAdd() {
        value::Value right = pop();
        value::Value left = pop();
        push(performBinaryOp(left, right, bytecode::OpCode::ADD));
    }

    void VirtualMachine::handleSub() {
        value::Value right = pop();
        value::Value left = pop();
        push(performBinaryOp(left, right, bytecode::OpCode::SUB));
    }

    void VirtualMachine::handleMul() {
        value::Value right = pop();
        value::Value left = pop();
        push(performBinaryOp(left, right, bytecode::OpCode::MUL));
    }

    void VirtualMachine::handleDiv() {
        value::Value right = pop();
        value::Value left = pop();
        push(performBinaryOp(left, right, bytecode::OpCode::DIV));
    }

    void VirtualMachine::handleMod() {
        value::Value right = pop();
        value::Value left = pop();
        push(performBinaryOp(left, right, bytecode::OpCode::MOD));
    }

    void VirtualMachine::handleNeg() {
        value::Value val = pop();
        if (std::holds_alternative<int>(val)) {
            push(-std::get<int>(val));
        } else if (std::holds_alternative<float>(val)) {
            push(-std::get<float>(val));
        } else {
            throw errors::RuntimeException("NEG requires numeric value");
        }
    }

    void VirtualMachine::handleAddInt() {
        checkStackUnderflow(2);
        int right = std::get<int>(pop());
        int left = std::get<int>(pop());
        push(left + right);
    }

    void VirtualMachine::handleSubInt() {
        checkStackUnderflow(2);
        int right = std::get<int>(pop());
        int left = std::get<int>(pop());
        push(left - right);
    }

    void VirtualMachine::handleMulInt() {
        checkStackUnderflow(2);
        int right = std::get<int>(pop());
        int left = std::get<int>(pop());
        push(left * right);
    }

    void VirtualMachine::handleDivInt() {
        checkStackUnderflow(2);
        int right = std::get<int>(pop());
        int left = std::get<int>(pop());
        if (right == 0) {
            throw errors::RuntimeException("Division by zero");
        }
        push(left / right);
    }

    // === Comparison Operations ===

    void VirtualMachine::handleEq() {
        value::Value right = pop();
        value::Value left = pop();
        // Simple equality for now - needs enhancement for objects
        push(left == right);
    }

    void VirtualMachine::handleNe() {
        value::Value right = pop();
        value::Value left = pop();
        push(left != right);
    }

    void VirtualMachine::handleLt() {
        value::Value right = pop();
        value::Value left = pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            push(std::get<int>(left) < std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            push(std::get<float>(left) < std::get<float>(right));
        } else {
            throw errors::RuntimeException("LT requires numeric operands");
        }
    }

    void VirtualMachine::handleGt() {
        value::Value right = pop();
        value::Value left = pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            push(std::get<int>(left) > std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            push(std::get<float>(left) > std::get<float>(right));
        } else {
            throw errors::RuntimeException("GT requires numeric operands");
        }
    }

    void VirtualMachine::handleLe() {
        value::Value right = pop();
        value::Value left = pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            push(std::get<int>(left) <= std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            push(std::get<float>(left) <= std::get<float>(right));
        } else {
            throw errors::RuntimeException("LE requires numeric operands");
        }
    }

    void VirtualMachine::handleGe() {
        value::Value right = pop();
        value::Value left = pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            push(std::get<int>(left) >= std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            push(std::get<float>(left) >= std::get<float>(right));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
    }

    // === Logical Operations ===

    void VirtualMachine::handleAnd() {
        bool right = isTruthy(pop());
        bool left = isTruthy(pop());
        push(left && right);
    }

    void VirtualMachine::handleOr() {
        bool right = isTruthy(pop());
        bool left = isTruthy(pop());
        push(left || right);
    }

    void VirtualMachine::handleNot() {
        push(!isTruthy(pop()));
    }

    // === Variable Operations ===

    void VirtualMachine::handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("LOAD_VAR requires operand");
        }
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);
        auto varDef = environment->findVariable(varName);
        if (!varDef) {
            throw errors::RuntimeException("Variable not found: " + varName);
        }
        push(varDef->getValue());
    }

    void VirtualMachine::handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("STORE_VAR requires operand");
        }
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);
        value::Value val = pop();
        auto varDef = environment->findVariable(varName);
        if (!varDef) {
            throw errors::RuntimeException("Variable not found: " + varName);
        }
        varDef->setValue(val);
    }

    void VirtualMachine::handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);
        value::Value val = pop();

        // Determine type from value
        value::ValueType type = value::getValueType(val);

        // Create variable definition
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            varName, type, val, false);

        environment->declareVariable(varName, varDef);
    }

    void VirtualMachine::handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("LOAD_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];

        // Get the current frame base (or 0 if no call frame)
        size_t frameBase = callStack.empty() ? 0 : callStack.back().localBase;

        // Calculate absolute stack position
        size_t stackPos = frameBase + slot;

        if (stackPos >= operandStack.size()) {
            throw errors::RuntimeException("Local variable slot out of bounds: " + std::to_string(slot));
        }

        // Load value from stack
        value::Value val = operandStack[stackPos];
        push(val);
    }

    void VirtualMachine::handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("STORE_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];

        // Get the current frame base (or 0 if no call frame)
        size_t frameBase = callStack.empty() ? 0 : callStack.back().localBase;

        // Calculate absolute stack position
        size_t stackPos = frameBase + slot;

        // Pop value from top of stack
        value::Value val = pop();

        // Extend stack if needed to reach the slot position
        while (operandStack.size() < stackPos) {
            operandStack.push_back(std::monostate{});
        }

        // If the slot is beyond current stack size, push the value
        if (stackPos >= operandStack.size()) {
            operandStack.push_back(val);
        } else {
            // Otherwise, store at the specific position
            operandStack[stackPos] = val;
        }
    }

    // === Control Flow Operations ===

    void VirtualMachine::handleJump(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP requires operand");
        }
        instructionPointer = instr.operands[0] - 1;  // -1 because loop increments
    }

    void VirtualMachine::handleJumpIfFalse(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_FALSE requires operand");
        }
        value::Value condition = pop();
        if (!isTruthy(condition)) {
            instructionPointer = instr.operands[0] - 1;
        }
    }

    void VirtualMachine::handleJumpIfTrue(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_TRUE requires operand");
        }
        value::Value condition = pop();
        if (isTruthy(condition)) {
            instructionPointer = instr.operands[0] - 1;
        }
    }

    void VirtualMachine::handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_BACK requires operand");
        }
        // Jump back to loop start (operand is the target instruction)
        instructionPointer = instr.operands[0] - 1;  // -1 because loop increments
    }

    void VirtualMachine::handleReturn() {
        if (callStack.empty()) {
            // Top-level return - halt execution
            instructionPointer = program->getInstructionCount();
        } else {
            CallFrame frame = callStack.back();
            callStack.pop_back();
            instructionPointer = frame.returnAddress;
            // Restore stack
            while (operandStack.size() > frame.frameBase) {
                operandStack.pop_back();
            }
        }
    }

    void VirtualMachine::handleReturnValue() {
        value::Value returnVal = pop();
        handleReturn();
        push(returnVal);
    }

    // === Function Operations ===

    void VirtualMachine::handleCall(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get function name from constant pool
        std::string functionName = program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Try to find native function first
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName)) {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc) {
                value::Value result = nativeFunc(args);
                push(result);
                return;
            }
        }

        // Try to find user-defined function
        auto funcDef = environment->getFunctionRegistry()->findFunction(functionName);
        if (funcDef) {
            // TODO: Implement user-defined function calls with call frames
            throw errors::RuntimeException("User-defined function calls not yet implemented in VM");
        }

        throw errors::RuntimeException("Function not found: " + functionName);
    }

    void VirtualMachine::handleCallNative(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement native function calls
        throw errors::RuntimeException("CALL_NATIVE not yet implemented");
    }

    // === Object Operations ===

    void VirtualMachine::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement object creation
        throw errors::RuntimeException("NEW_OBJECT not yet implemented");
    }

    void VirtualMachine::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement field access
        throw errors::RuntimeException("GET_FIELD not yet implemented");
    }

    void VirtualMachine::handleSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement field assignment
        throw errors::RuntimeException("SET_FIELD not yet implemented");
    }

    void VirtualMachine::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement method calls
        throw errors::RuntimeException("CALL_METHOD not yet implemented");
    }

    // === Array Operations ===

    void VirtualMachine::handleNewArray(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement array creation
        throw errors::RuntimeException("NEW_ARRAY not yet implemented");
    }

    void VirtualMachine::handleArrayGet() {
        // TODO: Implement array element access
        throw errors::RuntimeException("ARRAY_GET not yet implemented");
    }

    void VirtualMachine::handleArraySet() {
        // TODO: Implement array element assignment
        throw errors::RuntimeException("ARRAY_SET not yet implemented");
    }

    void VirtualMachine::handleArrayLength() {
        // TODO: Implement array length
        throw errors::RuntimeException("ARRAY_LENGTH not yet implemented");
    }

    // === Type Operations ===

    void VirtualMachine::handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement instanceof check
        throw errors::RuntimeException("INSTANCEOF not yet implemented");
    }

    void VirtualMachine::handleCast(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement type casting
        throw errors::RuntimeException("CAST not yet implemented");
    }

    // === Helper Methods ===

    bool VirtualMachine::isTruthy(const value::Value& val) const {
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int>(val)) {
            return std::get<int>(val) != 0;
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return false;
        }
        if (std::holds_alternative<std::monostate>(val)) {
            return false;
        }
        return true;  // Objects, strings, etc. are truthy
    }

    std::string VirtualMachine::valueToString(const value::Value& val) const {
        if (std::holds_alternative<int>(val)) {
            return std::to_string(std::get<int>(val));
        }
        if (std::holds_alternative<float>(val)) {
            return std::to_string(std::get<float>(val));
        }
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::string>(val)) {
            return std::get<std::string>(val);
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return "null";
        }
        return "<object>";
    }

    value::Value VirtualMachine::performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op) {
        using OpCode = bytecode::OpCode;

        // Integer operations
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            int l = std::get<int>(left);
            int r = std::get<int>(right);
            switch (op) {
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
            (std::holds_alternative<float>(right) || std::holds_alternative<int>(right))) {
            float l = std::holds_alternative<float>(left) ? std::get<float>(left) : static_cast<float>(std::get<int>(left));
            float r = std::holds_alternative<float>(right) ? std::get<float>(right) : static_cast<float>(std::get<int>(right));
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                    return l / r;
                default: break;
            }
        }

        // String concatenation
        if (op == OpCode::ADD &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right))) {
            return valueToString(left) + valueToString(right);
        }

        throw errors::RuntimeException("Invalid binary operation");
    }

    void VirtualMachine::checkStackUnderflow(size_t required) const {
        if (operandStack.size() < required) {
            throw errors::RuntimeException("Stack underflow: required " +
                std::to_string(required) + " but only " +
                std::to_string(operandStack.size()) + " available");
        }
    }

    const VirtualMachine::ExecutionStats& VirtualMachine::getStats() const {
        return stats;
    }

    std::vector<std::string> VirtualMachine::getStackTrace() const {
        std::vector<std::string> trace;
        for (const auto& frame : callStack) {
            std::ostringstream oss;
            oss << frame.functionName << " at offset " << frame.returnAddress;
            trace.push_back(oss.str());
        }
        return trace;
    }

    void VirtualMachine::reset() {
        operandStack.clear();
        callStack.clear();
        instructionPointer = 0;
        stats = ExecutionStats{};
    }
}
