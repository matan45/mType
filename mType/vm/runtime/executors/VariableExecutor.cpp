#include "VariableExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"

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

            // Check if we're in a method/constructor and should access a field from 'this'
            if (!context.callStack.empty() && context.callStack.back().thisInstance) {
                auto thisInstance = context.callStack.back().thisInstance;
                auto fieldDef = thisInstance->getField(varName);
                if (fieldDef) {
                    // Field found - load its value
                    value::Value fieldValue = thisInstance->getFieldValue(varName);
                    context.stackManager->push(fieldValue);
                    return;
                }
            }

            // Check if we're in a static method and should access a static field
            if (!context.callStack.empty()) {
                const std::string& functionName = context.callStack.back().functionName;
                // Static method names have format: ClassName::methodName
                size_t colonPos = functionName.find("::");
                if (colonPos != std::string::npos) {
                    std::string className = functionName.substr(0, colonPos);
                    auto classRegistry = context.environment->getClassRegistry();
                    if (classRegistry) {
                        auto classDef = classRegistry->findClass(className);
                        if (classDef) {
                            const auto& staticFields = classDef->getStaticFields();
                            auto it = staticFields.find(varName);
                            if (it != staticFields.end()) {
                                value::Value fieldValue = it->second->getValue();
                                context.stackManager->push(fieldValue);
                                return;
                            }
                        }
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
            // Check if we're in a method/constructor and should set a field on 'this'
            if (!context.callStack.empty() && context.callStack.back().thisInstance) {
                auto thisInstance = context.callStack.back().thisInstance;
                auto fieldDef = thisInstance->getField(varName);
                if (fieldDef) {
                    // Field found - check if it's final
                    if (fieldDef->isFinal()) {
                        throw errors::RuntimeException("Cannot assign to final field '" + varName + "'");
                    }
                    // Set the field value
                    thisInstance->setField(varName, val);
                    // Push value back for assignment expressions (e.g., x = y = 5)
                    context.stackManager->push(val);
                    return;
                }
            }

            // Check if we're in a static method and should set a static field
            if (!context.callStack.empty()) {
                const std::string& functionName = context.callStack.back().functionName;
                // Static method names have format: ClassName::methodName
                size_t colonPos = functionName.find("::");
                if (colonPos != std::string::npos) {
                    std::string className = functionName.substr(0, colonPos);
                    auto classRegistry = context.environment->getClassRegistry();
                    if (classRegistry) {
                        auto classDef = classRegistry->findClass(className);
                        if (classDef) {
                            const auto& staticFields = classDef->getStaticFields();
                            auto it = staticFields.find(varName);
                            if (it != staticFields.end()) {
                                // Check if it's final
                                if (it->second->isFinal()) {
                                    throw errors::RuntimeException("Cannot assign to final static field '" + varName + "'");
                                }
                                it->second->setValue(val);
                                // Push value back for assignment expressions (e.g., x = y = 5)
                                context.stackManager->push(val);
                                return;
                            }
                        }
                    }
                }
            }

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
