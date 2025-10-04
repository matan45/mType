#include "VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
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
            case OpCode::INC: handleInc(); break;
            case OpCode::DEC: handleDec(); break;
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
            case OpCode::CALL_STATIC: handleCallStatic(instr); break;

            // Objects
            case OpCode::NEW_OBJECT: handleNewObject(instr); break;
            case OpCode::GET_FIELD: handleGetField(instr); break;
            case OpCode::SET_FIELD: handleSetField(instr); break;
            case OpCode::GET_STATIC: handleGetStatic(instr); break;
            case OpCode::SET_STATIC: handleSetStatic(instr); break;
            case OpCode::CALL_METHOD: handleCallMethod(instr); break;
            case OpCode::SUPER_CONSTRUCTOR: handleSuperConstructor(instr); break;
            case OpCode::SUPER_INVOKE: handleSuperInvoke(instr); break;

            // Arrays
            case OpCode::NEW_ARRAY: handleNewArray(instr); break;
            case OpCode::NEW_ARRAY_MULTI: handleNewArrayMulti(instr); break;
            case OpCode::ARRAY_GET: handleArrayGet(); break;
            case OpCode::ARRAY_SET: handleArraySet(); break;
            case OpCode::ARRAY_LENGTH: handleArrayLength(); break;

            // Type operations
            case OpCode::INSTANCEOF: handleInstanceof(instr); break;
            case OpCode::CAST: handleCast(instr); break;

            // Lambda operations
            case OpCode::LAMBDA: handleLambda(instr); break;
            case OpCode::LAMBDA_INVOKE: handleLambdaInvoke(instr); break;

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

    void VirtualMachine::handleInc() {
        value::Value val = pop();
        if (std::holds_alternative<int>(val)) {
            push(std::get<int>(val) + 1);
        } else if (std::holds_alternative<float>(val)) {
            push(std::get<float>(val) + 1.0f);
        } else {
            throw errors::RuntimeException("INC requires numeric value");
        }
    }

    void VirtualMachine::handleDec() {
        value::Value val = pop();
        if (std::holds_alternative<int>(val)) {
            push(std::get<int>(val) - 1);
        } else if (std::holds_alternative<float>(val)) {
            push(std::get<float>(val) - 1.0f);
        } else {
            throw errors::RuntimeException("DEC requires numeric value");
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

        // Handle bool-to-int conversion for comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<int>(right)) {
            push(static_cast<int>(std::get<bool>(left)) == std::get<int>(right));
        } else if (std::holds_alternative<int>(left) && std::holds_alternative<bool>(right)) {
            push(std::get<int>(left) == static_cast<int>(std::get<bool>(right)));
        } else {
            // Direct equality comparison
            push(left == right);
        }
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
        // Check if variable is final
        if (varDef->isFinal()) {
            throw errors::RuntimeException("Cannot assign to final variable '" + varName + "'");
        }
        varDef->setValue(val);
        // Push value back for assignment expressions (e.g., x = y = 5)
        push(val);
    }

    void VirtualMachine::handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);
        value::Value val = pop();

        // Determine type from value
        value::ValueType type = value::getValueType(val);

        // Check if variable is final (third operand)
        bool isFinal = false;
        if (instr.operands.size() >= 3) {
            isFinal = (instr.operands[2] != 0);
        }

        // Create variable definition
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            varName, type, val, isFinal);

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
            // Exit function scope (to clean up parameters and local variables)
            environment->exitScope();
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

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = operandStack.size() - argCount;

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

        // Try to find user-defined function in bytecode
        auto funcMetadata = program->getFunction(functionName);
        if (funcMetadata) {
            // Create call frame
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = frameBase;  // Use the frameBase calculated before popping args
            frame.localBase = operandStack.size();  // Locals start after arguments (which are now popped)
            frame.functionName = functionName;
            frame.thisInstance = nullptr;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Push arguments onto operand stack as locals (they will be at frameBase + slot)
            for (size_t i = 0; i < argCount; ++i) {
                push(args[i]);
            }

            // Jump to function start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
            return;
        }

        throw errors::RuntimeException("Function not found: " + functionName);
    }

    void VirtualMachine::handleCallNative(const bytecode::BytecodeProgram::Instruction& instr) {
        // TODO: Implement native function calls
        throw errors::RuntimeException("CALL_NATIVE not yet implemented");
    }

    void VirtualMachine::handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get static method name from constant pool (should be fully qualified: ClassName::methodName)
        std::string qualifiedName = program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Look up static method bytecode
        auto funcMetadata = program->getFunction(qualifiedName);
        if (funcMetadata) {
            // Create call frame for static method
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = operandStack.size();
            frame.localBase = operandStack.size();
            frame.functionName = qualifiedName;
            frame.thisInstance = nullptr;  // No 'this' for static methods

            callStack.push_back(frame);
            stats.functionCalls++;

            // Create a new scope for the static method
            environment->enterScope();

            // Declare method parameters
            for (size_t i = 0; i < argCount && i < funcMetadata->parameterNames.size(); ++i) {
                const std::string& paramName = funcMetadata->parameterNames[i];
                const value::Value& argValue = args[i];
                value::ValueType type = value::getValueType(argValue);

                auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    paramName, type, argValue, false);
                environment->declareVariable(paramName, varDef);
            }

            // Jump to static method start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            throw errors::RuntimeException("Static method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    // === Object Operations ===

    void VirtualMachine::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("NEW_OBJECT requires 2 operands: class name index and arg count");
        }

        // Get class name from constant pool (may include generics like "Box<Int>")
        const std::string& fullClassName = program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Parse generic type arguments from className
        std::string baseClassName = fullClassName;
        std::unordered_map<std::string, std::string> genericTypeBindings;

        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genericStart);

            // Extract type arguments: "Box<Int>" -> ["Int"], "Map<String,Int>" -> ["String", "Int"]
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd != std::string::npos && genericEnd > genericStart) {
                std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

                // For now, we store the full type string as a binding
                // The runtime type resolution system will handle this
                // Example: T -> Int, K -> String, V -> Int
            }
        }

        // Pop constructor arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Get class definition from environment
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available");
        }

        // Look up the base class (without generic parameters)
        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + baseClassName);
        }

        // Create new object instance with generic type bindings
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef, genericTypeBindings);

        // Initialize instance fields with default values
        for (const auto& [fieldName, fieldDef] : classDef->getInstanceFields()) {
            instance->setField(fieldName, nullptr);  // Initialize to null
        }

        // Find and call constructor
        auto constructor = classDef->findConstructor(argCount);
        if (!constructor) {
            // No matching constructor found
            // Check if class has ANY constructors defined
            bool hasAnyConstructor = !classDef->getConstructors().empty();

            if (argCount == 0 && !hasAnyConstructor) {
                // Class has no constructors at all - use implicit default constructor
                push(instance);
                return;
            }

            // Either argCount != 0 or class has constructors but none match
            throw errors::RuntimeException("No constructor found for " + baseClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Call constructor - it will initialize the object and return it
        // For bytecode: Look up constructor function by name "<init>/<paramCount>"
        std::string constructorName = "<init>/" + std::to_string(argCount);
        auto funcMetadata = program->getFunction(constructorName);
        if (funcMetadata) {
            // Create call frame
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = operandStack.size();
            frame.localBase = operandStack.size();
            frame.functionName = "<init>";
            frame.thisInstance = instance;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Push 'this' onto stack as first local (slot 0)
            push(instance);

            // Push constructor arguments onto stack as locals (slot 1, 2, ...)
            for (size_t i = 0; i < argCount; ++i) {
                push(args[i]);
            }

            // Jump to constructor start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            // VM requires bytecode - no AST fallback
            throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName + "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }
    }

    void VirtualMachine::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("GET_FIELD requires operand");
        }

        // Get field name from constant pool
        const std::string& fieldName = program->getConstantPool().getString(instr.operands[0]);

        // Pop object from stack
        value::Value objectValue = pop();

        // Check for null
        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("GET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        // Get field definition to check access modifiers
        auto fieldDef = instance->getField(fieldName);
        if (fieldDef) {
            // TODO: Check access modifiers (public/private/protected)
            // For now, we allow all access (similar to AST interpreter default behavior)
            // Access modifier checks would be:
            // - PUBLIC: Always accessible
            // - PRIVATE: Only accessible from same class
            // - PROTECTED: Accessible from same class and subclasses
        }

        // Get field value
        value::Value fieldValue = instance->getFieldValue(fieldName);
        push(fieldValue);
    }

    void VirtualMachine::handleSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("SET_FIELD requires operand");
        }

        // Get field name from constant pool
        const std::string& fieldName = program->getConstantPool().getString(instr.operands[0]);

        // Pop value and object from stack
        value::Value fieldValue = pop();
        value::Value objectValue = pop();

        // Check for null
        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("SET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        // Set field value
        instance->setField(fieldName, fieldValue);

        // Push value back for assignment expressions
        push(fieldValue);
    }

    void VirtualMachine::handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("GET_STATIC requires operand");
        }

        // Get variable/field name from constant pool
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);

        // Find variable in environment
        auto varManager = environment->getVariableManager();
        auto varDef = varManager->findVariable(varName);

        if (!varDef) {
            throw errors::RuntimeException("Static variable not found: " + varName);
        }

        // Get and push the value
        value::Value varValue = varDef->getValue();
        push(varValue);
    }

    void VirtualMachine::handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("SET_STATIC requires operand");
        }

        // Get variable/field name from constant pool
        const std::string& varName = program->getConstantPool().getString(instr.operands[0]);

        // Pop value from stack
        value::Value varValue = pop();

        // Find or create variable in environment
        auto varManager = environment->getVariableManager();
        auto varDef = varManager->findVariable(varName);

        if (!varDef) {
            // Determine type from the value
            value::ValueType varType = value::ValueType::OBJECT; // Default to OBJECT

            if (std::holds_alternative<int>(varValue)) {
                varType = value::ValueType::INT;
            } else if (std::holds_alternative<float>(varValue)) {
                varType = value::ValueType::FLOAT;
            } else if (std::holds_alternative<bool>(varValue)) {
                varType = value::ValueType::BOOL;
            } else if (std::holds_alternative<std::string>(varValue) ||
                       std::holds_alternative<value::InternedString>(varValue)) {
                varType = value::ValueType::STRING;
            }

            // Create new global variable with inferred type
            varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                varName, varType, varValue, false);
            varManager->declareGlobalVariable(varName, varDef);
        } else {
            // Variable already exists, update its value
            varDef->setValue(varValue);
        }

        // Push value back for assignment expressions
        push(varValue);
    }

    void VirtualMachine::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("CALL_METHOD requires 2 operands");
        }

        // Get method name and argument count
        const std::string& methodName = program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Pop object from stack
        value::Value objectValue = pop();

        // Check for null
        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("CALL_METHOD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        // Find method in class hierarchy
        auto classDef = instance->getClassDefinition();
        auto method = classDef->findMethod(methodName, argCount);

        if (!method) {
            throw errors::RuntimeException("Method not found: " + methodName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Look up method in bytecode functions
        // Build qualified method name (ClassName::methodName)
        std::string className = classDef->getName();
        std::string qualifiedMethodName = className + "::" + methodName;
        auto funcMetadata = program->getFunction(qualifiedMethodName);
        if (funcMetadata) {
            // Calculate frameBase BEFORE any stack modifications
            size_t frameBase = operandStack.size();

            // Create call frame
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = operandStack.size();
            frame.functionName = qualifiedMethodName;
            frame.thisInstance = instance;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Push 'this' onto stack as first local (slot 0)
            push(instance);

            // Push method arguments onto stack as locals (slot 1, 2, ...)
            for (size_t i = 0; i < argCount; ++i) {
                push(args[i]);
            }

            // Jump to method start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            // Try to call as native/interpreted method
            value::Value result = instance->callMethod(methodName, args);
            push(result);
        }
    }

    void VirtualMachine::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR requires operand (argument count)");
        }

        size_t argCount = instr.operands[0];

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Get current 'this' instance from the environment
        auto thisVar = environment->findVariable("this");
        if (!thisVar || !std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(thisVar->getValue())) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR: 'this' not found or not an object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(thisVar->getValue());

        // Get parent class
        auto classDef = instance->getClassDefinition();
        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR: class has no parent");
        }

        std::string parentClassName = classDef->getParentClassName();
        auto classRegistry = environment->getClassRegistry();
        auto parentClassDef = classRegistry->findClass(parentClassName);

        if (!parentClassDef) {
            throw errors::RuntimeException("Parent class not found: " + parentClassName);
        }

        // Find parent constructor with matching argument count
        auto parentConstructor = parentClassDef->findConstructor(argCount);
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + parentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Look up parent constructor bytecode
        std::string constructorName = "<init>/" + std::to_string(argCount);
        auto funcMetadata = program->getFunction(constructorName);
        if (funcMetadata) {
            // Create call frame for parent constructor
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = operandStack.size();
            frame.localBase = operandStack.size();
            frame.functionName = "<init>";
            frame.thisInstance = instance;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Create a new scope for parent constructor
            environment->enterScope();

            // Declare 'this' parameter (same instance)
            auto thisDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                "this", value::ValueType::OBJECT, instance, false);
            environment->declareVariable("this", thisDef);

            // Declare constructor parameters
            for (size_t i = 0; i < argCount && i < funcMetadata->parameterNames.size() - 1; ++i) {
                const std::string& paramName = funcMetadata->parameterNames[i + 1];  // Skip 'this'
                const value::Value& argValue = args[i];
                value::ValueType type = value::getValueType(argValue);

                auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    paramName, type, argValue, false);
                environment->declareVariable(paramName, varDef);
            }

            // Jump to parent constructor start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            throw errors::RuntimeException("Parent constructor '" + constructorName +
                                         "' for class '" + parentClassName +
                                         "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }
    }

    void VirtualMachine::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("SUPER_INVOKE requires 2 operands: method name index and argument count");
        }

        // Get method name and argument count
        const std::string& methodName = program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Get current 'this' instance from the environment
        auto thisVar = environment->findVariable("this");
        if (!thisVar || !std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(thisVar->getValue())) {
            throw errors::RuntimeException("SUPER_INVOKE: 'this' not found or not an object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(thisVar->getValue());

        // Get parent class
        auto classDef = instance->getClassDefinition();
        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("SUPER_INVOKE: class has no parent");
        }

        std::string parentClassName = classDef->getParentClassName();
        auto classRegistry = environment->getClassRegistry();
        auto parentClassDef = classRegistry->findClass(parentClassName);

        if (!parentClassDef) {
            throw errors::RuntimeException("Parent class not found: " + parentClassName);
        }

        // Find parent method with matching argument count
        auto parentMethod = parentClassDef->findMethod(methodName, argCount);
        if (!parentMethod) {
            throw errors::RuntimeException("Method not found in parent class " + parentClassName +
                                         ": " + methodName + " with " + std::to_string(argCount) + " arguments");
        }

        // Build qualified method name (ParentClass::methodName)
        std::string qualifiedMethodName = parentClassName + "::" + methodName;

        // Look up parent method bytecode
        auto funcMetadata = program->getFunction(qualifiedMethodName);
        if (funcMetadata) {
            // Create call frame for parent method
            CallFrame frame;
            frame.returnAddress = instructionPointer;
            frame.frameBase = operandStack.size();
            frame.localBase = operandStack.size();
            frame.functionName = qualifiedMethodName;
            frame.thisInstance = instance;

            callStack.push_back(frame);
            stats.functionCalls++;

            // Create a new scope for parent method
            environment->enterScope();

            // Declare 'this' parameter (same instance)
            auto thisDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                "this", value::ValueType::OBJECT, instance, false);
            environment->declareVariable("this", thisDef);

            // Declare method parameters
            for (size_t i = 0; i < argCount && i < funcMetadata->parameterNames.size() - 1; ++i) {
                const std::string& paramName = funcMetadata->parameterNames[i + 1];  // Skip 'this'
                const value::Value& argValue = args[i];
                value::ValueType type = value::getValueType(argValue);

                auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    paramName, type, argValue, false);
                environment->declareVariable(paramName, varDef);
            }

            // Jump to parent method start
            instructionPointer = funcMetadata->startOffset - 1;  // -1 because loop will increment
        } else {
            throw errors::RuntimeException("Parent method '" + qualifiedMethodName +
                                         "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    // === Array Operations ===

    void VirtualMachine::handleNewArray(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = program->getConstantPool().getString(instr.operands[0]);

        // Pop array size from stack
        value::Value sizeVal = pop();
        int size = std::get<int>(sizeVal);

        if (size < 0) {
            throw errors::RuntimeException("Array size cannot be negative: " + std::to_string(size));
        }

        // Create new NativeArray
        auto array = std::make_shared<value::NativeArray>(size, value::ValueType::OBJECT, elementTypeName);

        // Push array onto stack
        push(array);
    }

    void VirtualMachine::handleNewArrayMulti(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get element type name from constant pool
        const std::string& elementTypeName = program->getConstantPool().getString(instr.operands[0]);
        size_t dimensionCount = instr.operands[1];

        // Pop dimension sizes from stack (in reverse order - last dimension first)
        std::vector<int> dimensions;
        dimensions.reserve(dimensionCount);
        for (size_t i = 0; i < dimensionCount; ++i) {
            value::Value sizeVal = pop();
            int size = std::get<int>(sizeVal);
            if (size < 0) {
                throw errors::RuntimeException("Array dimension size cannot be negative: " + std::to_string(size));
            }
            dimensions.push_back(size);
        }
        std::reverse(dimensions.begin(), dimensions.end());

        // For now, create using FlatMultiArray or nested arrays
        // Simple approach: create nested single-dimensional arrays
        // TODO: Use FlatMultiArray for better performance

        // Create the innermost arrays first, then work outward
        std::shared_ptr<value::NativeArray> result = createNestedArray(dimensions, 0, elementTypeName);
        push(result);
    }

    void VirtualMachine::handleArrayGet() {
        // Pop index from stack
        value::Value indexVal = pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                         " for array of size " + std::to_string(array->size()));
        }

        // Get element and push onto stack
        value::Value element = array->get(index);
        push(element);
    }

    void VirtualMachine::handleArraySet() {
        // Pop value from stack
        value::Value valueToSet = pop();

        // Pop index from stack
        value::Value indexVal = pop();
        int index = std::get<int>(indexVal);

        // Pop array from stack
        value::Value arrayVal = pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Bounds check
        if (index < 0 || static_cast<size_t>(index) >= array->size()) {
            throw errors::RuntimeException("Array index out of bounds: " + std::to_string(index) +
                                         " for array of size " + std::to_string(array->size()));
        }

        // Set element
        array->set(index, valueToSet);
    }

    void VirtualMachine::handleArrayLength() {
        // Pop array from stack
        value::Value arrayVal = pop();
        auto array = std::get<std::shared_ptr<value::NativeArray>>(arrayVal);

        // Get length and push as integer onto stack
        int length = static_cast<int>(array->size());
        push(length);
    }

    // === Type Operations ===

    void VirtualMachine::handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get target type name from constant pool
        const std::string& targetTypeName = program->getConstantPool().getString(instr.operands[0]);

        // Pop value to check
        value::Value val = pop();

        bool result = false;

        // Check type based on target type
        if (targetTypeName == "Int" || targetTypeName == "int") {
            result = std::holds_alternative<int>(val);
        }
        else if (targetTypeName == "Float" || targetTypeName == "float") {
            result = std::holds_alternative<float>(val);
        }
        else if (targetTypeName == "Bool" || targetTypeName == "bool") {
            result = std::holds_alternative<bool>(val);
        }
        else if (targetTypeName == "String" || targetTypeName == "string") {
            result = std::holds_alternative<std::string>(val) || std::holds_alternative<value::InternedString>(val);
        }
        else {
            // Object type check
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

                if (obj) {
                    auto classDef = obj->getClassDefinition();
                    std::string className = classDef->getName();

                    // Extract base class name from generic instantiation (e.g., "Box<Int>" -> "Box")
                    std::string baseClassName = className;
                    size_t genericStart = className.find('<');
                    if (genericStart != std::string::npos) {
                        baseClassName = className.substr(0, genericStart);
                    }

                    // Extract base target name from generic (e.g., "Box<Int>" -> "Box")
                    std::string baseTargetName = targetTypeName;
                    genericStart = targetTypeName.find('<');
                    if (genericStart != std::string::npos) {
                        baseTargetName = targetTypeName.substr(0, genericStart);
                    }

                    // Check if object's class matches target type (exact or base generic match)
                    result = (className == targetTypeName) || (baseClassName == baseTargetName);

                    // If not exact match, check inheritance hierarchy
                    if (!result) {
                        result = classDef->isSubclassOf(targetTypeName);

                        // Also check with base names for generic types
                        if (!result && baseTargetName != targetTypeName) {
                            result = classDef->isSubclassOf(baseTargetName);
                        }
                    }

                    // Also check if object implements an interface with that name
                    if (!result) {
                        const auto& interfaces = classDef->getImplementedInterfaces();
                        for (const auto& iface : interfaces) {
                            // Extract base interface name for comparison
                            std::string baseIfaceName = iface;
                            size_t ifaceGenericStart = iface.find('<');
                            if (ifaceGenericStart != std::string::npos) {
                                baseIfaceName = iface.substr(0, ifaceGenericStart);
                            }

                            if (iface == targetTypeName || baseIfaceName == baseTargetName) {
                                result = true;
                                break;
                            }
                        }
                    }
                } else {
                    result = false; // null is not instance of any type
                }
            }
            else if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
                result = false; // null is not instance of any type
            }
            else {
                result = false; // primitive values are not instances of object types
            }
        }

        // Push boolean result onto stack
        push(result);
    }

    void VirtualMachine::handleCast(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get target type name from constant pool
        const std::string& targetTypeName = program->getConstantPool().getString(instr.operands[0]);

        // Pop value to cast
        value::Value val = pop();

        // Perform casting based on target type
        if (targetTypeName == "Int" || targetTypeName == "int") {
            // Cast to int
            if (std::holds_alternative<int>(val)) {
                push(val); // Already int
            }
            else if (std::holds_alternative<float>(val)) {
                push(static_cast<int>(std::get<float>(val)));
            }
            else if (std::holds_alternative<bool>(val)) {
                push(std::get<bool>(val) ? 1 : 0);
            }
            else if (std::holds_alternative<std::string>(val)) {
                try {
                    push(std::stoi(std::get<std::string>(val)));
                } catch (...) {
                    throw errors::RuntimeException("Cannot cast string to int: " + std::get<std::string>(val));
                }
            }
            else {
                throw errors::RuntimeException("Cannot cast to int from this type");
            }
        }
        else if (targetTypeName == "Float" || targetTypeName == "float") {
            // Cast to float
            if (std::holds_alternative<float>(val)) {
                push(val); // Already float
            }
            else if (std::holds_alternative<int>(val)) {
                push(static_cast<float>(std::get<int>(val)));
            }
            else if (std::holds_alternative<std::string>(val)) {
                try {
                    push(std::stof(std::get<std::string>(val)));
                } catch (...) {
                    throw errors::RuntimeException("Cannot cast string to float: " + std::get<std::string>(val));
                }
            }
            else {
                throw errors::RuntimeException("Cannot cast to float from this type");
            }
        }
        else if (targetTypeName == "String" || targetTypeName == "string") {
            // Cast to string
            push(valueToString(val));
        }
        else {
            // Object cast - check if it's a valid object type
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

                if (obj) {
                    auto classDef = obj->getClassDefinition();
                    std::string className = classDef->getName();

                    // Extract base class name and type parameters from object's class
                    std::string baseClassName = className;
                    std::string classTypeParams = "";
                    size_t genericStart = className.find('<');
                    if (genericStart != std::string::npos) {
                        baseClassName = className.substr(0, genericStart);
                        classTypeParams = className.substr(genericStart); // includes <>
                    }

                    // Extract base target name and type parameters
                    std::string baseTargetName = targetTypeName;
                    std::string targetTypeParams = "";
                    genericStart = targetTypeName.find('<');
                    if (genericStart != std::string::npos) {
                        baseTargetName = targetTypeName.substr(0, genericStart);
                        targetTypeParams = targetTypeName.substr(genericStart); // includes <>
                    }

                    // Check if the object's class can be cast to target type
                    bool canCast = false;

                    // 1. Exact match (including generic type parameters)
                    if (className == targetTypeName) {
                        canCast = true;
                    }
                    // 2. Base class match without type parameters (e.g., Box<Int> -> Box)
                    else if (baseClassName == baseTargetName && targetTypeParams.empty()) {
                        canCast = true;
                    }
                    // 3. Upcast - object is subclass of target type
                    else if (classDef->isSubclassOf(targetTypeName)) {
                        canCast = true;
                    }
                    // 4. Upcast with base names (for generic types)
                    else if (classDef->isSubclassOf(baseTargetName)) {
                        // Check if type parameters match (if target has type params)
                        if (!targetTypeParams.empty() && !classTypeParams.empty()) {
                            // Type parameters must match for generic upcast
                            canCast = (classTypeParams == targetTypeParams);
                        } else {
                            // No type params on target, allow upcast to generic base
                            canCast = true;
                        }
                    }
                    // 5. Downcast - target type is subclass of object's type (runtime check needed)
                    else {
                        // Get target class from registry to check if it's a subclass
                        auto targetClass = environment->findClass(baseTargetName);
                        if (targetClass) {
                            // Check if target class is a subclass of the object's actual class
                            if (targetClass->isSubclassOf(baseClassName)) {
                                // Check type parameter compatibility for generic downcast
                                if (!targetTypeParams.empty() && !classTypeParams.empty()) {
                                    // Type parameters must match for valid generic downcast
                                    canCast = (classTypeParams == targetTypeParams);
                                } else {
                                    // No type param mismatch, allow downcast
                                    canCast = true;
                                }
                            }
                        }
                    }
                    // 6. Interface check
                    if (!canCast) {
                        const auto& interfaces = classDef->getImplementedInterfaces();
                        for (const auto& iface : interfaces) {
                            // Extract base interface name
                            std::string baseIfaceName = iface;
                            std::string ifaceTypeParams = "";
                            size_t ifaceGenericStart = iface.find('<');
                            if (ifaceGenericStart != std::string::npos) {
                                baseIfaceName = iface.substr(0, ifaceGenericStart);
                                ifaceTypeParams = iface.substr(ifaceGenericStart);
                            }

                            // Check exact match or base match
                            if (iface == targetTypeName ||
                                (baseIfaceName == baseTargetName && targetTypeParams.empty())) {
                                canCast = true;
                                break;
                            }
                            // Check type param compatibility
                            if (baseIfaceName == baseTargetName && !targetTypeParams.empty()) {
                                canCast = (ifaceTypeParams == targetTypeParams);
                                if (canCast) break;
                            }
                        }
                    }

                    if (canCast) {
                        push(obj);
                    } else {
                        throw errors::RuntimeException(
                            "Cannot cast " + className + " to " + targetTypeName +
                            ": incompatible types in inheritance hierarchy"
                        );
                    }
                } else {
                    throw errors::RuntimeException("Cannot cast null to " + targetTypeName);
                }
            }
            else if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
                push(val); // null remains null for object casts
            }
            else {
                throw errors::RuntimeException("Cannot cast primitive type to " + targetTypeName);
            }
        }
    }

    // === Lambda Operations ===

    void VirtualMachine::handleLambda(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get lambda function start address and parameter count
        size_t lambdaStart = instr.operands[0];
        size_t paramCount = instr.operands[1];

        // Create bytecode lambda
        auto lambda = std::make_shared<BytecodeLambda>();
        lambda->instructionPointer = lambdaStart;
        lambda->parameterCount = paramCount;

        // Push lambda value onto stack
        push(lambda);
    }

    void VirtualMachine::handleLambdaInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        // Pop argument count
        size_t argCount = instr.operands[0];

        // Pop arguments from stack
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(pop());
        }
        std::reverse(args.begin(), args.end());

        // Pop lambda value from stack
        value::Value lambdaVal = pop();
        auto lambda = std::get<std::shared_ptr<BytecodeLambda>>(lambdaVal);

        size_t lambdaStart = lambda->instructionPointer;
        size_t paramCount = lambda->parameterCount;

        // Validate argument count
        if (args.size() != paramCount) {
            throw errors::RuntimeException("Lambda expects " + std::to_string(paramCount) +
                                         " arguments but got " + std::to_string(args.size()));
        }

        // Create call frame
        CallFrame frame;
        frame.returnAddress = instructionPointer;  // Return to next instruction
        frame.frameBase = operandStack.size();
        frame.localBase = operandStack.size();
        frame.functionName = "<lambda>";

        callStack.push_back(frame);

        // Push arguments onto stack (they become local variables)
        for (const auto& arg : args) {
            push(arg);
        }

        // Jump to lambda start
        instructionPointer = lambdaStart;
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

    std::shared_ptr<value::NativeArray> VirtualMachine::createNestedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size()) {
            throw errors::RuntimeException("Invalid dimension index in multi-dimensional array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1) {
            // Last dimension - create array of actual elements
            return std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, elementTypeName);
        }
        else {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i) {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }
}
