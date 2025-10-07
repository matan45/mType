#include "VariableExecutor.hpp"

namespace vm::runtime
{
    VariableExecutor::VariableExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void VariableExecutor::handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("LOAD_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        auto varDef = context.environment->findVariable(varName);
        if (!varDef) {
            // Variable not found in global environment
            // Check if we're in a lambda with a parent frame (for late-bound variable access)
            if (!context.callStack.empty() && context.callStack.back().originatingLambda) {
                auto lambda = context.callStack.back().originatingLambda;
                if (lambda->parentFrame) {
                    value::Value val = lambda->parentFrame->getLocalByName(varName);
                    if (!std::holds_alternative<std::monostate>(val)) {
                        context.stackManager->push(val);
                        return;
                    }
                }
            }
            throw errors::RuntimeException("Variable not found: " + varName);
        }
        context.stackManager->push(varDef->getValue());
    }

    void VariableExecutor::handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("STORE_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value val = context.stackManager->pop();
        auto varDef = context.environment->findVariable(varName);
        if (!varDef) {
            throw errors::RuntimeException("Variable not found: " + varName);
        }
        // Check if variable is final
        if (varDef->isFinal()) {
            throw errors::RuntimeException("Cannot assign to final variable '" + varName + "'");
        }
        varDef->setValue(val);
        // Push value back for assignment expressions (e.g., x = y = 5)
        context.stackManager->push(val);
    }

    void VariableExecutor::handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value val = context.stackManager->pop();

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

        context.environment->declareVariable(varName, varDef);
    }

    void VariableExecutor::handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("LOAD_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];

        // Get the current frame base (or 0 if no call frame)
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;

        // Calculate absolute stack position
        size_t stackPos = frameBase + slot;

        if (stackPos >= context.stackManager->size()) {
            throw errors::RuntimeException("Local variable slot out of bounds: " + std::to_string(slot));
        }

        // Load value from stack
        value::Value val = (*context.stackManager)[stackPos];
        context.stackManager->push(val);
    }

    void VariableExecutor::handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("STORE_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];
        std::string varName = "";
        if (instr.operands.size() > 1) {
            // Variable name provided (for shared frame late-binding)
            varName = context.program->getConstantPool().getString(instr.operands[1]);
        }

        // Get the current frame base (or 0 if no call frame)
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;

        // Calculate absolute stack position
        size_t stackPos = frameBase + slot;

        // Pop value from top of stack
        value::Value val = context.stackManager->pop();

        // Extend stack if needed to reach the slot position
        while (context.stackManager->size() < stackPos) {
            context.stackManager->getStack().push_back(std::monostate{});
        }

        // If the slot is beyond current stack size, push the value
        if (stackPos >= context.stackManager->size()) {
            context.stackManager->getStack().push_back(val);
        } else {
            // Otherwise, store at the specific position
            (*context.stackManager)[stackPos] = val;
        }

        // Also update the shared frame if one exists (for lambda late-binding)
        // This ensures lambdas see updated values of variables (including forward references)
        if (!context.callStack.empty() && context.callStack.back().sharedFrame) {
            if (!varName.empty()) {
                // Register the variable name -> slot mapping
                context.callStack.back().sharedFrame->setLocal(varName, slot, val);
            } else {
                context.callStack.back().sharedFrame->setLocal(slot, val);
            }
        }
    }
}
