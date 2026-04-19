#include "VariableExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../value/ValueTypeUtils.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../constants/SecurityConstants.hpp"
namespace vm::runtime
{
    VariableExecutor::VariableExecutor(ExecutionContext& ctx)
        : context(ctx)
    {
    }

    bool VariableExecutor::tryLoadFromInstanceField(const std::string& varName) {
        // Check if we're in a method/constructor and should access a field from 'this'
        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            return false;
        }

        auto thisInstance = context.callStack.back().thisInstance;
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
        // Check if we're in a static method and should access a static field
        if (context.callStack.empty()) {
            return false;
        }

        const std::string& functionName = context.callStack.back().functionName;
        // Static method names have format: ClassName::methodName
        size_t colonPos = functionName.find("::");
        if (colonPos == std::string::npos) {
            return false;
        }

        std::string className = functionName.substr(0, colonPos);
        auto classRegistry = context.environment->getClassRegistry();
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
        if (instr.operands.empty()) {
            throw errors::RuntimeException("LOAD_VAR requires operand");
        }

        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        auto varDef = context.environment->findVariable(varName);

        // Found in global environment
        if (varDef) {
            context.stackManager->push(varDef->getValue());
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
        // Check if we're in a method/constructor and should set a field on 'this'
        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            return false;
        }

        auto thisInstance = context.callStack.back().thisInstance;
        auto fieldDef = thisInstance->getField(varName);
        if (!fieldDef) {
            return false;
        }

        // Field found - check if it's final. Static finals share a single
        // isInitialized bit on FieldDefinition; instance finals track state
        // per-instance in the fieldValues map (MYT-189: match ObjectExecutor's
        // SET_FIELD check so per-instance field init doesn't trip the check
        // after the first instance sets the field).
        if (fieldDef->isFinal()) {
            if (fieldDef->isStatic()) {
                if (fieldDef->isInitialized()) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + varName + "'");
                }
            } else {
                const auto& instanceFields = thisInstance->getAllFieldValues();
                if (instanceFields.find(varName) != instanceFields.end()) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + varName + "'");
                }
            }
        }

        // Set the field value
        thisInstance->setField(varName, val);
        // Push value back for assignment expressions (e.g., x = y = 5)
        context.stackManager->push(val);
        return true;
    }

    bool VariableExecutor::tryStoreToStaticField(const std::string& varName, const value::Value& val) {
        // Check if we're in a static method and should set a static field
        if (context.callStack.empty()) {
            return false;
        }

        const std::string& functionName = context.callStack.back().functionName;
        // Static method names have format: ClassName::methodName
        size_t colonPos = functionName.find("::");
        if (colonPos == std::string::npos) {
            return false;
        }

        std::string className = functionName.substr(0, colonPos);
        auto classRegistry = context.environment->getClassRegistry();
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

        // Check if it's final
        if (it->second->isFinal() && it->second->isInitialized()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Cannot assign to final static field '" + varName + "'");
        }

        it->second->setValue(val);
        // Push value back for assignment expressions (e.g., x = y = 5)
        context.stackManager->push(val);
        return true;
    }

    void VariableExecutor::validateAndStoreGlobalVariable(const std::string& varName,
                                                          const value::Value& val,
                                                          std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef) {
        // Check if variable is final
        if (varDef->isFinal()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Cannot assign to final variable '" + varName + "'");
        }

        varDef->setValue(val);

        // Globals don't need SharedStackFrame updates
        // They are accessible through the Environment

        // Push value back for assignment expressions (e.g., x = y = 5)
        context.stackManager->push(val);
    }

    void VariableExecutor::handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("STORE_VAR requires operand");
        }

        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value val = context.stackManager->pop();
        auto varDef = context.environment->findVariable(varName);

        // Found in global environment
        if (varDef) {
            validateAndStoreGlobalVariable(varName, val, varDef);
            return;
        }

        // Try alternative variable storage methods
        if (tryStoreToInstanceField(varName, val)) return;
        if (tryStoreToStaticField(varName, val)) return;

        // Variable not found anywhere
        utils::ErrorLocationHelper::throwRuntimeError(context,
            "Variable not found: " + varName);
    }

    void VariableExecutor::handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty())
        {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value val = context.stackManager->pop();

        // Determine type from value
        value::ValueType type = value::ValueTypeUtils::getValueType(val);

        // Check if variable is final (third operand)
        bool isFinal = false;
        if (instr.operands.size() >= 3)
        {
            isFinal = (instr.operands[2] != 0);
        }

        // Create variable definition
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            varName, type, val, isFinal);

        context.environment->declareVariable(varName, varDef);

        // Globals don't need to be registered in SharedStackFrame
        // They are accessible through the Environment, not through closure capture
    }

    void VariableExecutor::handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty())
        {
            throw errors::RuntimeException("LOAD_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];

        // SECURITY: cap the slot index symmetrically with handleStoreLocal.
        // The attacker controls `slot` via bytecode operands; without this
        // cap a near-SIZE_MAX value would wrap when added to frameBase
        // below (line `frameBase + slot`) and could land inside a valid
        // stack range, bypassing the bounds check that follows.
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL slot index " + std::to_string(slot) +
                " exceeds per-frame limit " +
                std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
        }

        // Check if we're in a lambda context and this is a captured variable
        // If so, look it up by name through the SharedStackFrame parent chain for reference capture
        if (!context.callStack.empty() && context.callStack.back().originatingLambda)
        {
            auto lambda = context.callStack.back().originatingLambda;
            size_t paramCount = lambda->parameterCount;
            size_t capturedCount = lambda->capturedSlots.size();

            // Check if this slot is in the captured variable range
            if (slot >= paramCount && slot < paramCount + capturedCount)
            {
                // This is a captured variable - look it up by name through parent chain
                size_t capturedIndex = slot - paramCount;
                if (capturedIndex < lambda->capturedNames.size())
                {
                    // Use the slot number directly instead of looking up by name
                    // This allows multiple variables with the same name to coexist
                    size_t capturedSlot = lambda->capturedSlots[capturedIndex];

                    if (lambda->capturedFrame)
                    {
                        // Access by slot number (reference capture)
                        value::Value val = lambda->capturedFrame->getLocal(capturedSlot);

                        if (!value::isVoid(val))
                        {
                            context.stackManager->push(val);
                            return;
                        }
                    }
                }
            }
        }

        // Check if we're in an outer function that has captured this variable
        // If so, read from SharedStackFrame for reference semantics
        if (!context.callStack.empty() && context.callStack.back().sharedFrame)
        {
            auto sharedFrame = context.callStack.back().sharedFrame;
            value::Value sharedVal = sharedFrame->getLocal(slot);

            // If the variable exists in the shared frame, use it (reference capture)
            if (!value::isVoid(sharedVal))
            {
                context.stackManager->push(sharedVal);
                return;
            }
        }

        // Normal local variable access - read from stack
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;

        if (stackPos >= context.stackManager->size())
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Local variable slot out of bounds: " + std::to_string(slot));
        }

        // Load value from stack
        value::Value val = (*context.stackManager)[stackPos];
        context.stackManager->push(val);
    }

    void VariableExecutor::handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty())
        {
            throw errors::RuntimeException("STORE_LOCAL requires operand");
        }

        size_t slot = instr.operands[0];

        // SECURITY: cap the slot index directly. The attacker controls `slot`
        // via bytecode operands, so an unbounded value would otherwise drive
        // the stack-extending while loop below into an OOM/DoS condition.
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "STORE_LOCAL slot index " + std::to_string(slot) +
                " exceeds per-frame limit " +
                std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
        }

        std::string varName = "";
        if (instr.operands.size() > 1)
        {
            // Variable name provided (for shared frame late-binding)
            varName = context.program->getConstantPool().getString(instr.operands[1]);
        }

        // Pop value from top of stack
        value::Value val = context.stackManager->pop();

        // Check if we're in a lambda context and this is a captured variable
        // If so, update it through the SharedStackFrame parent chain for reference capture
        if (!context.callStack.empty() && context.callStack.back().originatingLambda)
        {
            auto lambda = context.callStack.back().originatingLambda;
            size_t paramCount = lambda->parameterCount;
            size_t capturedCount = lambda->capturedSlots.size();

            // Check if this slot is in the captured variable range
            if (slot >= paramCount && slot < paramCount + capturedCount)
            {
                // This is a captured variable - update it by name through parent chain
                size_t capturedIndex = slot - paramCount;
                if (capturedIndex < lambda->capturedNames.size())
                {
                    std::string capturedVarName = lambda->capturedNames[capturedIndex];

                    if (!capturedVarName.empty() && lambda->capturedFrame)
                    {
                        // Update through parent chain (reference capture)
                        lambda->capturedFrame->setLocalByName(capturedVarName, val);
                        // Push value back for assignment expressions
                        context.stackManager->push(val);
                        return;
                    }
                }
            }
        }

        // Normal local variable access - write to stack
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;

        // Extend stack if needed to reach the slot position
        while (context.stackManager->size() < stackPos)
        {
            context.stackManager->getStack().push_back(std::monostate{});  // Sentinel for uninitialized slots
        }

        // If the slot is beyond current stack size, push the value
        if (stackPos >= context.stackManager->size())
        {
            context.stackManager->getStack().push_back(val);
        }
        else
        {
            // Otherwise, store at the specific position
            (*context.stackManager)[stackPos] = val;
        }

        // Also update the SharedStackFrame (for closure capture reference semantics)
        // IMPORTANT: Always update SharedStackFrame if it exists, to support C# reference semantics
        // BUT: Only if we're NOT in a lambda context (lambdas handle their own captured var updates above)
        if (!context.callStack.empty() && !context.callStack.back().originatingLambda)
        {
            // If SharedStackFrame already exists, update it
            if (context.callStack.back().sharedFrame)
            {
                auto sharedFrame = context.callStack.back().sharedFrame;
                // Update using slot-based storage (always, for consistency)
                sharedFrame->setLocal(slot, val);
            }
            // If we have a varName and no SharedStackFrame yet, create one
            else if (!varName.empty())
            {
                context.callStack.back().sharedFrame = std::make_shared<SharedStackFrame>();
                auto sharedFrame = context.callStack.back().sharedFrame;
                sharedFrame->setLocal(varName, slot, val);
            }
        }

        // Push value back for assignment expressions (e.g., int i = 0 in for loop)
        context.stackManager->push(val);
    }
}
