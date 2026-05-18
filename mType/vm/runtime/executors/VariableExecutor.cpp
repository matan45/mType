#include "VariableExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include "../../../value/ObjectInstance.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../value/ValueTypeUtils.hpp"

namespace vm::runtime
{
    // MYT-319: local-variable hot paths live in VariableExecutor.hpp.
    // Global-variable handling stays here because it needs ObjectInstance /
    // ClassDefinition (instance-field and static-field fallbacks) plus
    // ValueTypeUtils for DECLARE_VAR.

    bool VariableExecutor::tryLoadFromInstanceField(const std::string& varName) {
        // Check if we're in a method/constructor and should access a field from 'this'.
        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames).
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            return false;
        }

        auto* thisInstance = context.callStack.back().getThisInstanceRaw();
        auto fieldDef = thisInstance->getField(varName);
        if (!fieldDef) {
            return false;
        }

        // Field found - load its value
        value::Value fieldValue = thisInstance->getFieldValue(varName);
        context.stackManager->push(fieldValue);
        return true;
    }

    bool VariableExecutor::tryLoadFromStaticField(const std::string& varName) {
        if (context.callStack.empty()) {
            return false;
        }

        // MYT-197: prefer frame.definingClassName.
        std::string className;
        if (!context.callStack.back().definingClassName.empty()) {
            className = context.callStack.back().definingClassName;
        } else {
            const std::string& functionName = context.program->getFrameName(context.callStack.back().functionName);
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) {
                return false;
            }
            className = functionName.substr(0, colonPos);
        }
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            return false;
        }

        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            return false;
        }

        const auto& staticFields = classDef->getStaticFields();
        auto it = staticFields.find(varName);
        if (it == staticFields.end()) {
            return false;
        }

        value::Value fieldValue = it->second->getValue();
        context.stackManager->push(fieldValue);
        return true;
    }

    void VariableExecutor::handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-318: operand-count contract enforced by program-load validator.
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        auto varDef = environment->findVariable(varName);

        // Found in global environment
        if (varDef) {
            context.stackManager->push(varDef->getValue());
            // MYT-204: snapshot and rewrite to LOAD_VAR_CACHED.
            tryPromoteLoadVarCached(varDef.get());
            return;
        }

        // Try alternative variable access methods
        if (tryLoadFromInstanceField(varName)) return;
        if (tryLoadFromStaticField(varName)) return;

        // Variable not found anywhere
        utils::ErrorLocationHelper::throwRuntimeError(context,
            "Variable not found: " + varName);
    }

    bool VariableExecutor::tryStoreToInstanceField(const std::string& varName, const value::Value& val) {
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            return false;
        }

        auto* thisInstance = context.callStack.back().getThisInstanceRaw();
        auto fieldDef = thisInstance->getField(varName);
        if (!fieldDef) {
            return false;
        }

        // Field found - check if it's final.
        if (fieldDef->isFinal()) {
            if (fieldDef->isStatic()) {
                if (fieldDef->isInitialized()) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + varName + "'");
                }
            } else {
                size_t idx = thisInstance->getClassDefinition()->getFieldIndex(varName);
                if (idx != SIZE_MAX)
                {
                    if (!value::isVoid(thisInstance->getFieldByIndex(idx)))
                    {
                        utils::ErrorLocationHelper::throwRuntimeError(context,
                            "Cannot assign to final field '" + varName + "'");
                    }
                }
            }
        }

        // Set the field value
        thisInstance->setField(varName, val);
        context.stackManager->push(val);
        return true;
    }

    bool VariableExecutor::tryStoreToStaticField(const std::string& varName, const value::Value& val) {
        if (context.callStack.empty()) {
            return false;
        }

        std::string className;
        if (!context.callStack.back().definingClassName.empty()) {
            className = context.callStack.back().definingClassName;
        } else {
            const std::string& functionName = context.program->getFrameName(context.callStack.back().functionName);
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) {
                return false;
            }
            className = functionName.substr(0, colonPos);
        }
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            return false;
        }

        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            return false;
        }

        const auto& staticFields = classDef->getStaticFields();
        auto it = staticFields.find(varName);
        if (it == staticFields.end()) {
            return false;
        }

        if (it->second->isFinal() && it->second->isInitialized()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Cannot assign to final static field '" + varName + "'");
        }

        it->second->setValue(val);
        context.stackManager->push(val);
        return true;
    }

    void VariableExecutor::validateAndStoreGlobalVariable(const std::string& varName,
                                                          const value::Value& val,
                                                          std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef) {
        if (varDef->isFinal()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Cannot assign to final variable '" + varName + "'");
        }

        varDef->setValue(val);

        context.stackManager->push(val);

        // MYT-204: rewrite this site to STORE_VAR_CACHED.
        tryPromoteStoreVarCached(varDef.get());
    }

    void VariableExecutor::handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value val = context.stackManager->pop();
        auto varDef = environment->findVariable(varName);

        if (varDef) {
            validateAndStoreGlobalVariable(varName, val, varDef);
            return;
        }

        if (tryStoreToInstanceField(varName, val)) return;
        if (tryStoreToStaticField(varName, val)) return;

        utils::ErrorLocationHelper::throwRuntimeError(context,
            "Variable not found: " + varName);
    }

    void VariableExecutor::handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.numOperands() == 0)
        {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value val = context.stackManager->pop();

        value::ValueType type = value::ValueTypeUtils::getValueType(val);

        bool isFinal = false;
        if (instr.numOperands() >= 3)
        {
            isFinal = (instr.inlineOperands[2] != 0);
        }

        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            varName, type, val, isFinal);

        environment->declareVariable(varName, varDef);
    }

    // MYT-204 promote helpers.
    void VariableExecutor::tryPromoteLoadVarCached(
        runtimeTypes::global::VariableDefinition* slot)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        if (mut.opcode != bytecode::OpCode::LOAD_VAR) return;

        auto& state = context.getOrCreateCachedState(ip);
        state.cachedGlobalSlot = slot;
        mut.opcode = bytecode::OpCode::LOAD_VAR_CACHED;
    }

    void VariableExecutor::tryPromoteStoreVarCached(
        runtimeTypes::global::VariableDefinition* slot)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        if (mut.opcode != bytecode::OpCode::STORE_VAR) return;

        auto& state = context.getOrCreateCachedState(ip);
        state.cachedGlobalSlot = slot;
        mut.opcode = bytecode::OpCode::STORE_VAR_CACHED;
    }
}
