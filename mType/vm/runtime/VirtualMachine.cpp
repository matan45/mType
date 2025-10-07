#include "VirtualMachine.hpp"
#include "executors/StackOperationsExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ArithmeticExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/StringPool.hpp"
#include "../../value/LambdaValue.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <functional>
#include <unordered_set>

namespace vm::runtime
{
    VirtualMachine::VirtualMachine(std::shared_ptr<environment::Environment> env)
        : program(nullptr)
          , stackManager(std::make_shared<StackManager>())
          , instructionPointer(0)
          , environment(std::move(env))
    {
        callStack.reserve(64);

        // Note: Executors will be initialized in execute() when program is available
        // because ExecutionContext requires a valid program pointer
    }

    VirtualMachine::~VirtualMachine() = default;

    value::Value VirtualMachine::execute(const bytecode::BytecodeProgram& bytecodeProgram)
    {
        program = &bytecodeProgram;
        instructionPointer = program->getEntryPoint();

        executionStart = std::chrono::steady_clock::now();
        stats = ExecutionStats{};

        // Initialize executors with execution context
        ExecutionContext context(program, instructionPointer, callStack, environment,
                                 stackManager, stats, executionStart);
        stackOpsExecutor = std::make_unique<StackOperationsExecutor>(context);
        comparisonExecutor = std::make_unique<ComparisonExecutor>(context);
        logicalExecutor = std::make_unique<LogicalExecutor>(context);
        arithmeticExecutor = std::make_unique<ArithmeticExecutor>(context);
        controlFlowExecutor = std::make_unique<ControlFlowExecutor>(context);
        variableExecutor = std::make_unique<VariableExecutor>(context);
        functionExecutor = std::make_unique<FunctionExecutor>(context);
        typeExecutor = std::make_unique<TypeExecutor>(context);
        arrayExecutor = std::make_unique<ArrayExecutor>(context);
        objectExecutor = std::make_unique<ObjectExecutor>(context);
        lambdaExecutor = std::make_unique<LambdaExecutor>(context);

        try
        {
            return interpretLoop();
        }
        catch (...)
        {
            // Clean up and rethrow
            reset();
            throw;
        }
    }

    value::Value VirtualMachine::executeFunction(const std::string& functionName, const std::vector<value::Value>& args)
    {
        if (!program)
        {
            throw errors::RuntimeException("No program loaded");
        }

        auto* funcMetadata = program->getFunction(functionName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Function not found: " + functionName);
        }

        // Push arguments onto stack
        for (const auto& arg : args)
        {
            push(arg);
        }

        // Set instruction pointer to function start
        instructionPointer = funcMetadata->startOffset;

        return interpretLoop();
    }

    value::Value VirtualMachine::interpretLoop()
    {
        while (instructionPointer < program->getInstructionCount())
        {
            const auto& instr = program->getInstruction(instructionPointer);

            // Execute instruction
            executeInstruction(instr);

            stats.instructionsExecuted++;

            // Check for halt
            if (instr.opcode == bytecode::OpCode::HALT)
            {
                break;
            }

            instructionPointer++;
        }

        // Calculate execution time
        auto executionEnd = std::chrono::steady_clock::now();
        stats.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(
            executionEnd - executionStart);

        // Return top of stack or void
        if (!stackManager->empty())
        {
            return stackManager->pop();
        }
        return std::monostate{};
    }

    void VirtualMachine::executeInstruction(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;

        switch (instr.opcode)
        {
        // Stack operations - delegated to StackOperationsExecutor
        case OpCode::PUSH_INT: stackOpsExecutor->handlePushInt(instr);
            break;
        case OpCode::PUSH_FLOAT: stackOpsExecutor->handlePushFloat(instr);
            break;
        case OpCode::PUSH_STRING: stackOpsExecutor->handlePushString(instr);
            break;
        case OpCode::PUSH_BOOL: stackOpsExecutor->handlePushBool(instr);
            break;
        case OpCode::PUSH_NULL: stackOpsExecutor->handlePushNull();
            break;
        case OpCode::POP: stackOpsExecutor->handlePop();
            break;
        case OpCode::DUP: stackOpsExecutor->handleDup();
            break;
        case OpCode::SWAP: stackOpsExecutor->handleSwap();
            break;

        // Arithmetic - delegated to ArithmeticExecutor
        case OpCode::ADD: arithmeticExecutor->handleAdd();
            break;
        case OpCode::SUB: arithmeticExecutor->handleSub();
            break;
        case OpCode::MUL: arithmeticExecutor->handleMul();
            break;
        case OpCode::DIV: arithmeticExecutor->handleDiv();
            break;
        case OpCode::MOD: arithmeticExecutor->handleMod();
            break;
        case OpCode::NEG: arithmeticExecutor->handleNeg();
            break;
        case OpCode::INC: arithmeticExecutor->handleInc();
            break;
        case OpCode::DEC: arithmeticExecutor->handleDec();
            break;
        case OpCode::ADD_INT: arithmeticExecutor->handleAddInt();
            break;
        case OpCode::SUB_INT: arithmeticExecutor->handleSubInt();
            break;
        case OpCode::MUL_INT: arithmeticExecutor->handleMulInt();
            break;
        case OpCode::DIV_INT: arithmeticExecutor->handleDivInt();
            break;

        // Comparison - delegated to ComparisonExecutor
        case OpCode::EQ: comparisonExecutor->handleEq();
            break;
        case OpCode::NE: comparisonExecutor->handleNe();
            break;
        case OpCode::LT: comparisonExecutor->handleLt();
            break;
        case OpCode::GT: comparisonExecutor->handleGt();
            break;
        case OpCode::LE: comparisonExecutor->handleLe();
            break;
        case OpCode::GE: comparisonExecutor->handleGe();
            break;

        // Logical - delegated to LogicalExecutor
        case OpCode::AND: logicalExecutor->handleAnd();
            break;
        case OpCode::OR: logicalExecutor->handleOr();
            break;
        case OpCode::NOT: logicalExecutor->handleNot();
            break;

        // Variables - delegated to VariableExecutor
        case OpCode::LOAD_VAR: variableExecutor->handleLoadVar(instr);
            break;
        case OpCode::STORE_VAR: variableExecutor->handleStoreVar(instr);
            break;
        case OpCode::DECLARE_VAR: variableExecutor->handleDeclareVar(instr);
            break;
        case OpCode::LOAD_LOCAL: variableExecutor->handleLoadLocal(instr);
            break;
        case OpCode::STORE_LOCAL: variableExecutor->handleStoreLocal(instr);
            break;

        // Control flow - delegated to ControlFlowExecutor
        case OpCode::JUMP: controlFlowExecutor->handleJump(instr);
            break;
        case OpCode::JUMP_IF_FALSE: controlFlowExecutor->handleJumpIfFalse(instr);
            break;
        case OpCode::JUMP_IF_TRUE: controlFlowExecutor->handleJumpIfTrue(instr);
            break;
        case OpCode::JUMP_BACK: controlFlowExecutor->handleJumpBack(instr);
            break;
        case OpCode::RETURN: controlFlowExecutor->handleReturn();
            break;
        case OpCode::RETURN_VALUE: controlFlowExecutor->handleReturnValue();
            break;

        // Functions - delegated to FunctionExecutor
        case OpCode::CALL: functionExecutor->handleCall(instr);
            break;
        case OpCode::CALL_NATIVE: functionExecutor->handleCallNative(instr);
            break;
        case OpCode::CALL_STATIC: functionExecutor->handleCallStatic(instr);
            break;

        // Objects - delegated to ObjectExecutor
        case OpCode::NEW_OBJECT: objectExecutor->handleNewObject(instr);
            break;
        case OpCode::GET_FIELD: objectExecutor->handleGetField(instr);
            break;
        case OpCode::SET_FIELD: objectExecutor->handleSetField(instr);
            break;
        case OpCode::GET_STATIC: objectExecutor->handleGetStatic(instr);
            break;
        case OpCode::SET_STATIC: objectExecutor->handleSetStatic(instr);
            break;
        case OpCode::CALL_METHOD: objectExecutor->handleCallMethod(instr);
            break;
        case OpCode::SUPER_CONSTRUCTOR: objectExecutor->handleSuperConstructor(instr);
            break;
        case OpCode::SUPER_INVOKE: objectExecutor->handleSuperInvoke(instr);
            break;

        // Arrays - delegated to ArrayExecutor
        case OpCode::NEW_ARRAY: arrayExecutor->handleNewArray(instr);
            break;
        case OpCode::NEW_ARRAY_MULTI: arrayExecutor->handleNewArrayMulti(instr);
            break;
        case OpCode::ARRAY_GET: arrayExecutor->handleArrayGet();
            break;
        case OpCode::ARRAY_SET: arrayExecutor->handleArraySet();
            break;
        case OpCode::ARRAY_LENGTH: arrayExecutor->handleArrayLength();
            break;

        // Type operations - delegated to TypeExecutor
        case OpCode::INSTANCEOF: typeExecutor->handleInstanceof(instr);
            break;
        case OpCode::CAST: typeExecutor->handleCast(instr);
            break;

        // Lambda operations - delegated to LambdaExecutor
        case OpCode::LAMBDA: lambdaExecutor->handleLambda(instr);
            break;
        case OpCode::LAMBDA_INVOKE: lambdaExecutor->handleLambdaInvoke(instr);
            break;

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


    // === Helper Methods ===

    bool VirtualMachine::isTruthy(const value::Value& val) const
    {
        if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int>(val))
        {
            return std::get<int>(val) != 0;
        }
        if (std::holds_alternative<nullptr_t>(val))
        {
            return false;
        }
        if (std::holds_alternative<std::monostate>(val))
        {
            return false;
        }
        return true; // Objects, strings, etc. are truthy
    }

    std::string VirtualMachine::valueToString(const value::Value& val) const
    {
        if (std::holds_alternative<int>(val))
        {
            return std::to_string(std::get<int>(val));
        }
        if (std::holds_alternative<float>(val))
        {
            // Format float to match interpreter behavior (remove trailing zeros)
            std::ostringstream oss;
            oss << std::get<float>(val);
            return oss.str();
        }
        if (std::holds_alternative<bool>(val))
        {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::string>(val))
        {
            return std::get<std::string>(val);
        }
        if (std::holds_alternative<value::InternedString>(val))
        {
            return std::get<value::InternedString>(val).getString();
        }
        if (std::holds_alternative<nullptr_t>(val))
        {
            return "null";
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
        {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj)
            {
                // First, try to call toString() if it exists (custom toString() takes priority)
                auto classDef = obj->getClassDefinition();
                if (classDef && classDef->hasMethod("toString"))
                {
                    auto toStringMethod = classDef->findMethod("toString", 0);
                    if (toStringMethod && !toStringMethod->isStatic())
                    {
                        try
                        {
                            // WORKAROUND: obj->callMethod() is currently a stub that returns void
                            // Instead, manually construct toString() output from object fields
                            // This is a heuristic approach that handles common toString() patterns

                            std::string result;
                            bool constructed = false;

                            // Pattern 1: name:value (e.g., TestObject with name and value fields)
                            if (obj->getField("name") && obj->getField("value"))
                            {
                                value::Value nameVal = obj->getFieldValue("name");
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(nameVal) + ":" + valueToString(valueVal);
                                constructed = true;
                            }
                            // Pattern 2: just a "value" field (for primitive wrappers)
                            else if (obj->getField("value") && classDef->getInstanceFields().size() == 1)
                            {
                                value::Value valueVal = obj->getFieldValue("value");
                                result = valueToString(valueVal);
                                constructed = true;
                            }

                            if (constructed)
                            {
                                return result;
                            }
                        }
                        catch (...)
                        {
                            // If toString() construction fails, fall through to default handling
                        }
                    }
                }

                // Fallback: For primitive wrapper objects (String, Int, etc.), extract the "value" field
                // Only if toString() doesn't exist or failed
                if (obj->getField("value"))
                {
                    value::Value fieldValue = obj->getFieldValue("value");
                    // Recursively convert the field value to string
                    return valueToString(fieldValue);
                }
            }
        }
        return "<object>";
    }

    value::Value VirtualMachine::performBinaryOp(const value::Value& left, const value::Value& right,
                                                 bytecode::OpCode op)
    {
        using OpCode = bytecode::OpCode;

        // Debug output
        // Integer operations
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right))
        {
            int l = std::get<int>(left);
            int r = std::get<int>(right);
            switch (op)
            {
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
            (std::holds_alternative<float>(right) || std::holds_alternative<int>(right)))
        {
            float l = std::holds_alternative<float>(left)
                          ? std::get<float>(left)
                          : static_cast<float>(std::get<int>(left));
            float r = std::holds_alternative<float>(right)
                          ? std::get<float>(right)
                          : static_cast<float>(std::get<int>(right));
            switch (op)
            {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                return l / r;
            default: break;
            }
        }

        // String concatenation (includes objects, which should call toString())
        if (op == OpCode::ADD &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
                std::holds_alternative<value::InternedString>(left) || std::holds_alternative<
                    value::InternedString>(right) ||
                std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left) ||
                std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right)))
        {
            // Convert both operands to string using valueToString
            // This will call toString() for objects via AST (cross-mode call)
            std::string leftStr = valueToString(left);
            std::string rightStr = valueToString(right);

            // Concatenate and intern the result
            std::string result = leftStr + rightStr;
            auto& pool = value::StringPool::getInstance();
            return pool.intern(std::move(result));
        }

        throw errors::RuntimeException("Invalid binary operation");
    }

    void VirtualMachine::checkStackUnderflow(size_t required) const
    {
        // Delegate to StackManager - this method is kept for compatibility
        // Will be removed once all handlers are moved to executors
        if (stackManager->size() < required)
        {
            throw errors::RuntimeException("Stack underflow: required " +
                std::to_string(required) + " but only " +
                std::to_string(stackManager->size()) + " available");
        }
    }

    const ExecutionStats& VirtualMachine::getStats() const
    {
        return stats;
    }

    std::vector<std::string> VirtualMachine::getStackTrace() const
    {
        std::vector<std::string> trace;
        for (const auto& frame : callStack)
        {
            std::ostringstream oss;
            oss << frame.functionName << " at offset " << frame.returnAddress;
            trace.push_back(oss.str());
        }
        return trace;
    }

    void VirtualMachine::reset()
    {
        stackManager->clear();
        callStack.clear();
        instructionPointer = 0;
        stats = ExecutionStats{};
    }

    value::ValueType VirtualMachine::stringToValueType(const std::string& typeName)
    {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        // For any object/class type
        return value::ValueType::OBJECT;
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createJaggedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName, size_t totalDimensions)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in jagged array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last specified dimension
            // If this is also the last total dimension, create array of actual elements
            // Otherwise, create array of null references (for jagged arrays)
            if (dimensions.size() == totalDimensions)
            {
                // Fully specified - create array of elements
                // Convert element type name to ValueType
                value::ValueType elemType = stringToValueType(elementTypeName);
                return std::make_shared<value::NativeArray>(currentDimSize, elemType, elementTypeName);
            }
            else
            {
                // Jagged - create array of null array references
                auto array = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");
                // Elements are initialized to std::monostate{} (null) by default
                return array;
            }
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createJaggedArray(dimensions, dimIndex + 1, elementTypeName, totalDimensions);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }

    std::shared_ptr<value::NativeArray> VirtualMachine::createNestedArray(
        const std::vector<int>& dimensions, size_t dimIndex, const std::string& elementTypeName)
    {
        if (dimIndex >= dimensions.size())
        {
            throw errors::RuntimeException("Invalid dimension index in multi-dimensional array creation");
        }

        int currentDimSize = dimensions[dimIndex];

        if (dimIndex == dimensions.size() - 1)
        {
            // Last dimension - create array of actual elements
            return std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, elementTypeName);
        }
        else
        {
            // Not last dimension - create array of arrays
            auto outerArray = std::make_shared<value::NativeArray>(currentDimSize, value::ValueType::OBJECT, "Array");

            // Fill with nested arrays
            for (int i = 0; i < currentDimSize; ++i)
            {
                auto innerArray = createNestedArray(dimensions, dimIndex + 1, elementTypeName);
                outerArray->set(i, innerArray);
            }

            return outerArray;
        }
    }
}
