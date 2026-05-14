#include "VariableExecutor.hpp"
#include <cstddef>
#include <cstdint>
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
        // Check if we're in a method/constructor and should access a field from 'this'.
        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames) — the
        // raw pointer is interchangeable with the shared_ptr's underlying
        // pointer for field-access purposes.
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
        // Check if we're in a static method and should access a static field
        if (context.callStack.empty()) {
            return false;
        }

        // MYT-197: prefer frame.definingClassName (populated at push time)
        // over resolving the interned handle and re-splitting.
        std::string className;
        if (!context.callStack.back().definingClassName.empty()) {
            className = context.callStack.back().definingClassName;
        } else {
            const std::string& functionName = context.frameName(context.callStack.back());
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) {
                return false;
            }
            className = functionName.substr(0, colonPos);
        }
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
        // MYT-318: operand-count contract enforced by program-load validator.
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        auto varDef = context.environment->findVariable(varName);

        // Found in global environment
        if (varDef) {
            context.stackManager->push(varDef->getValue());
            // MYT-204: snapshot the VariableDefinition pointer and rewrite
            // this site to LOAD_VAR_CACHED so the next dispatch skips the
            // constant-pool fetch and the name-keyed Environment lookup.
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
        // Check if we're in a method/constructor and should set a field on 'this'.
        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            return false;
        }

        auto* thisInstance = context.callStack.back().getThisInstanceRaw();
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
        // Push value back for assignment expressions (e.g., x = y = 5)
        context.stackManager->push(val);
        return true;
    }

    bool VariableExecutor::tryStoreToStaticField(const std::string& varName, const value::Value& val) {
        // Check if we're in a static method and should set a static field
        if (context.callStack.empty()) {
            return false;
        }

        // MYT-197: prefer frame.definingClassName (populated at push time).
        std::string className;
        if (!context.callStack.back().definingClassName.empty()) {
            className = context.callStack.back().definingClassName;
        } else {
            const std::string& functionName = context.frameName(context.callStack.back());
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) {
                return false;
            }
            className = functionName.substr(0, colonPos);
        }
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

        // MYT-204: rewrite this site to STORE_VAR_CACHED. setValue mutates
        // the VariableDefinition's Value in place, so the cached pointer
        // continues to reflect future writes without re-resolution.
        tryPromoteStoreVarCached(varDef.get());
    }

    void VariableExecutor::handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-318: operand-count contract enforced by program-load validator.
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
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
        if (instr.numOperands() == 0)
        {
            throw errors::RuntimeException("DECLARE_VAR requires operand");
        }
        const std::string& varName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value val = context.stackManager->pop();

        // Determine type from value
        value::ValueType type = value::ValueTypeUtils::getValueType(val);

        // Check if variable is final (third operand)
        bool isFinal = false;
        if (instr.numOperands() >= 3)
        {
            isFinal = (instr.inlineOperands[2] != 0);
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
        // MYT-318: operand-count contract enforced by program-load validator.
        loadLocalSlot(instr.inlineOperands[0]);
    }

    void VariableExecutor::loadLocalSlot(size_t slot)
    {
        // SECURITY: cap the slot index symmetrically with storeLocalSlot.
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

        // MYT-199: observe the loaded type and, on a stable monomorphic
        // observation, promote this instruction to LOAD_LOCAL_<TAG>. Gated
        // on a simple frame: lambda / sharedFrame frames have divergent
        // read paths above that the specialized executor can't replicate.
        tryPromoteLoadLocal(val.tag());
    }

    void VariableExecutor::handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-318: operand-count contract (>= 1) enforced by program-load validator.

        // Fast path: no varName (shared-frame late-binding) — delegate to the
        // slot-based entry so MYT-202 fused handlers share the same body.
        if (instr.numOperands() == 1)
        {
            storeLocalSlot(instr.inlineOperands[0]);
            return;
        }

        size_t slot = instr.inlineOperands[0];

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

        std::string varName = context.program->getConstantPool().getString(instr.inlineOperands[1]);

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

        // MYT-199: observe the stored type and, on a stable monomorphic
        // observation, promote this instruction to STORE_LOCAL_<TAG>.
        tryPromoteStoreLocal(val.tag());
    }

    void VariableExecutor::storeLocalSlot(size_t slot)
    {
        // SECURITY: symmetric cap with handleStoreLocal (attacker-controlled
        // slot → unbounded stack extension would DoS).
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "STORE_LOCAL slot index " + std::to_string(slot) +
                " exceeds per-frame limit " +
                std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
        }

        // MYT-318 fast path: when the frame is non-lambda AND the target slot
        // is *strictly below* TOS, copy TOS into the slot. The original TOS
        // value stays in place to provide the chained-assignment semantic
        // (`int x = expr` evaluates to expr's value for further use), so we
        // skip both the pop and the push.
        //
        // The boundary case stackPos == curSize - 1 (local slot equals TOS)
        // requires duplicating the value — pushing val onto TOS again to
        // preserve the chained-val invariant. That case is handled by the
        // slow path below to keep this fast path branch-free on the common
        // function-body shape (locals are always below the operand work).
        const bool inLambda = !context.callStack.empty()
                              && context.callStack.back().originatingLambda;
        if (!inLambda)
        {
            size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
            size_t stackPos = frameBase + slot;
            size_t curSize = context.stackManager->size();
            if (curSize >= 2 && stackPos < curSize - 1)
            {
                const auto& tosRef = context.stackManager->peekRef(0);
                (*context.stackManager)[stackPos] = tosRef;
                if (!context.callStack.empty() && context.callStack.back().sharedFrame)
                {
                    context.callStack.back().sharedFrame->setLocal(slot, tosRef);
                }
                value::ValueType tag = tosRef.tag();
                // val stays on TOS for the chained-assignment semantic.
                tryPromoteStoreLocal(tag);
                return;
            }
            // Fall through to slow path for the boundary (stackPos == TOS)
            // and stack-extension cases.
        }

        value::Value val = context.stackManager->pop();

        // Lambda-captured path.
        if (inLambda)
        {
            auto lambda = context.callStack.back().originatingLambda;
            size_t paramCount = lambda->parameterCount;
            size_t capturedCount = lambda->capturedSlots.size();
            if (slot >= paramCount && slot < paramCount + capturedCount)
            {
                size_t capturedIndex = slot - paramCount;
                if (capturedIndex < lambda->capturedNames.size())
                {
                    std::string capturedVarName = lambda->capturedNames[capturedIndex];
                    if (!capturedVarName.empty() && lambda->capturedFrame)
                    {
                        lambda->capturedFrame->setLocalByName(capturedVarName, val);
                        context.stackManager->push(val);
                        return;
                    }
                }
            }
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;

        while (context.stackManager->size() < stackPos)
        {
            context.stackManager->getStack().push_back(std::monostate{});
        }
        if (stackPos >= context.stackManager->size())
        {
            context.stackManager->getStack().push_back(val);
        }
        else
        {
            (*context.stackManager)[stackPos] = val;
        }

        // Mirror into an existing SharedStackFrame. Skip the "create on
        // varName" path — fused dispatch never carries a varName, and the
        // handleStoreLocal varName branch above remains the only creator.
        if (!context.callStack.empty() && !context.callStack.back().originatingLambda
            && context.callStack.back().sharedFrame)
        {
            context.callStack.back().sharedFrame->setLocal(slot, val);
        }

        context.stackManager->push(val);
        tryPromoteStoreLocal(val.tag());
    }

    // ------------------------------------------------------------------
    // MYT-199: type-quickening helpers and specialized handlers.
    // ------------------------------------------------------------------

    bool VariableExecutor::isCurrentFrameSimple() const
    {
        // Top-level script frame has no capture machinery — treat as simple.
        if (context.callStack.empty()) return true;
        const auto& frame = context.callStack.back();
        // Lambda bodies route LOAD_LOCAL through capturedFrame — we can't
        // replicate that here, so lambda frames are always non-simple.
        if (frame.originatingLambda) return false;
        // sharedFrame with use_count == 1 is speculative bookkeeping:
        // handleStoreLocal allocates one on every store-with-varName in case
        // a lambda later captures the slot, but no external holder exists
        // yet. Mirrors OSRManager::captureState's check — only treat the
        // frame as non-simple when a lambda (or interop path) is actually
        // holding a reference (use_count > 1).
        if (frame.sharedFrame && frame.sharedFrame.use_count() > 1) return false;
        return true;
    }

    void VariableExecutor::tryPromoteLoadLocal(value::ValueType observedTag)
    {
        const size_t ip = context.instructionPointer;

        // MYT-202: only promote if the IP actually holds a generic LOAD_LOCAL.
        // handleLoadLocal can be dispatched as a shim by fused-opcode handlers
        // (LOAD_GET_FIELD, LOAD_LOAD_ADD_INT, …), in which case the IP holds
        // the fused opcode — rewriting that would destroy the fusion. This
        // invariant check makes the rewrite contract explicit: we only
        // specialize the exact opcode we were dispatched as.
        if (context.getMutableInstructionAt(ip).opcode != bytecode::OpCode::LOAD_LOCAL)
            return;

        // Sticky demote — once the site has deopted, it stays on the generic
        // path for good. Mirrors InlineCacheExecutor::tryPromoteToCached.
        // MYT-201: read via findCachedState so a never-observed site doesn't
        // allocate an entry just to check the default 0.
        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }
        if (!isCurrentFrameSimple()) return;

        bytecode::OpCode specialized;
        switch (observedTag)
        {
        case value::ValueType::INT:
            specialized = bytecode::OpCode::LOAD_LOCAL_INT;
            break;
        case value::ValueType::FLOAT:
            specialized = bytecode::OpCode::LOAD_LOCAL_FLOAT;
            break;
        case value::ValueType::BOOL:
            specialized = bytecode::OpCode::LOAD_LOCAL_BOOL;
            break;
        case value::ValueType::OBJECT:
            specialized = bytecode::OpCode::LOAD_LOCAL_BOXED_INST;
            break;
        default:
            // VOID / NULL_TYPE / STRING / VALUE_OBJECT / ARRAY / LAMBDA /
            // PROMISE — no specialized variant for MVP. Leave generic.
            return;
        }

        // observedValueType records the first observation for invariant /
        // debug purposes. The specialized dispatch path reads `expectedTag`
        // from the opcode itself, not from this field, so a mismatch on a
        // later dispatch (which only happens after a sticky demote re-entry)
        // is caught by the cachedDeoptCount gate above.
        auto& state = context.getOrCreateCachedState(ip);
        state.observedValueType = static_cast<uint8_t>(observedTag);

        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = specialized;
    }

    void VariableExecutor::tryPromoteStoreLocal(value::ValueType observedTag)
    {
        const size_t ip = context.instructionPointer;

        // MYT-202: see tryPromoteLoadLocal.
        if (context.getMutableInstructionAt(ip).opcode != bytecode::OpCode::STORE_LOCAL)
            return;

        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }
        if (!isCurrentFrameSimple()) return;

        bytecode::OpCode specialized;
        switch (observedTag)
        {
        case value::ValueType::INT:
            specialized = bytecode::OpCode::STORE_LOCAL_INT;
            break;
        case value::ValueType::FLOAT:
            specialized = bytecode::OpCode::STORE_LOCAL_FLOAT;
            break;
        case value::ValueType::BOOL:
            specialized = bytecode::OpCode::STORE_LOCAL_BOOL;
            break;
        case value::ValueType::OBJECT:
            specialized = bytecode::OpCode::STORE_LOCAL_BOXED_INST;
            break;
        default:
            return;
        }
        auto& state = context.getOrCreateCachedState(ip);
        state.observedValueType = static_cast<uint8_t>(observedTag);

        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = specialized;
    }

    void VariableExecutor::deoptLoadLocal(const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        // Rewrite opcode back to generic and mark sticky. Re-enter the
        // generic handler so the current dispatch completes correctly with
        // the observed value. MYT-201: entry is guaranteed to exist here —
        // a specialized opcode implies prior promote.
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = bytecode::OpCode::LOAD_LOCAL;
        auto& state = context.getOrCreateCachedState(ip);
        if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
        handleLoadLocal(mut);
    }

    void VariableExecutor::deoptStoreLocal(const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = bytecode::OpCode::STORE_LOCAL;
        auto& state = context.getOrCreateCachedState(ip);
        if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
        handleStoreLocal(mut);
    }

    void VariableExecutor::handleLoadLocalSpecialized(
        const bytecode::BytecodeProgram::Instruction& instr,
        value::ValueType expectedTag)
    {
        if (instr.numOperands() == 0)
        {
            throw errors::RuntimeException("LOAD_LOCAL requires operand");
        }

        size_t slot = instr.inlineOperands[0];
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL slot index " + std::to_string(slot) +
                " exceeds per-frame limit " +
                std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
        }

        // Non-simple frames (lambda body / shared-frame function) require the
        // capture-aware read paths in handleLoadLocal. We didn't promote from
        // these, but this Instruction is shared across all invocations of
        // this IP — if a later call happens to enter via a lambda/shared
        // frame, defer to the generic handler. No sticky bump: the frame
        // condition is orthogonal to the type observation.
        if (!isCurrentFrameSimple())
        {
            handleLoadLocal(instr);
            return;
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Local variable slot out of bounds: " + std::to_string(slot));
        }

        const value::Value& val = (*context.stackManager)[stackPos];
        if (val.tag() != expectedTag)
        {
            // Type guard miss — stickily demote and let the generic handler
            // finish this dispatch on the correct path.
            deoptLoadLocal(instr);
            return;
        }

        // Tag matched — this is the MYT-199 fast path. The read skips the
        // lambda / shared-frame probes above, and the pushed value carries
        // a known-good tag so future fusion (MYT-198 successors) can elide
        // their own isInt / isFloat / isBool / isObject checks.
        context.stackManager->push(val);
    }

    void VariableExecutor::handleStoreLocalSpecialized(
        const bytecode::BytecodeProgram::Instruction& instr,
        value::ValueType expectedTag)
    {
        if (instr.numOperands() == 0)
        {
            throw errors::RuntimeException("STORE_LOCAL requires operand");
        }

        size_t slot = instr.inlineOperands[0];
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "STORE_LOCAL slot index " + std::to_string(slot) +
                " exceeds per-frame limit " +
                std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
        }

        if (!isCurrentFrameSimple())
        {
            handleStoreLocal(instr);
            return;
        }

        // Peek first so a type miss leaves the stack intact for the generic
        // handler to re-run on deopt (mirrors handleCallMethodCached).
        if (context.stackManager->size() == 0)
        {
            deoptStoreLocal(instr);
            return;
        }
        const value::Value& tos = context.stackManager->peek(0);
        if (tos.tag() != expectedTag)
        {
            deoptStoreLocal(instr);
            return;
        }

        // Keep the varName resolution exactly as the generic handler — the
        // sharedFrame pre-allocation that happens on the first STORE of a
        // named slot is relied on by LAMBDA / CAPTURE_VARIABLE (the compiler
        // emits STORE_LOCAL with operand[1]=nameIndex precisely so that a
        // lambda defined later can capture the slot).
        std::string varName;
        if (instr.numOperands() > 1)
        {
            varName = context.program->getConstantPool().getString(instr.inlineOperands[1]);
        }

        value::Value val = context.stackManager->pop();
        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;

        while (context.stackManager->size() < stackPos)
        {
            context.stackManager->getStack().push_back(std::monostate{});
        }
        if (stackPos >= context.stackManager->size())
        {
            context.stackManager->getStack().push_back(val);
        }
        else
        {
            (*context.stackManager)[stackPos] = val;
        }

        // SharedStackFrame propagation — identical to handleStoreLocal's
        // post-write block. isCurrentFrameSimple() has already ruled out
        // originatingLambda, so we don't duplicate that check here. The
        // existing sharedFrame may have been pre-allocated by a prior
        // (pre-promotion) store; keep it in sync so a lambda captured later
        // sees current values.
        if (!context.callStack.empty())
        {
            if (context.callStack.back().sharedFrame)
            {
                context.callStack.back().sharedFrame->setLocal(slot, val);
            }
            else if (!varName.empty())
            {
                context.callStack.back().sharedFrame = std::make_shared<SharedStackFrame>();
                context.callStack.back().sharedFrame->setLocal(varName, slot, val);
            }
        }

        context.stackManager->push(val);
    }

    void VariableExecutor::handleLoadLocalInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleLoadLocalSpecialized(instr, value::ValueType::INT);
    }

    void VariableExecutor::handleLoadLocalFloat(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleLoadLocalSpecialized(instr, value::ValueType::FLOAT);
    }

    void VariableExecutor::handleLoadLocalBool(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleLoadLocalSpecialized(instr, value::ValueType::BOOL);
    }

    void VariableExecutor::handleLoadLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleLoadLocalSpecialized(instr, value::ValueType::OBJECT);
    }

    void VariableExecutor::handleStoreLocalInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleStoreLocalSpecialized(instr, value::ValueType::INT);
    }

    void VariableExecutor::handleStoreLocalFloat(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleStoreLocalSpecialized(instr, value::ValueType::FLOAT);
    }

    void VariableExecutor::handleStoreLocalBool(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleStoreLocalSpecialized(instr, value::ValueType::BOOL);
    }

    void VariableExecutor::handleStoreLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr)
    {
        handleStoreLocalSpecialized(instr, value::ValueType::OBJECT);
    }

    // ------------------------------------------------------------------
    // MYT-204: LOAD_VAR_CACHED / STORE_VAR_CACHED.
    // ------------------------------------------------------------------

    void VariableExecutor::tryPromoteLoadVarCached(
        runtimeTypes::global::VariableDefinition* slot)
    {
        const size_t ip = context.instructionPointer;
        // Only promote if IP currently holds a fresh LOAD_VAR. Guards against
        // re-promoting a site that has already been rewritten to _CACHED, and
        // against accidentally clobbering a fused superinstruction whose
        // handler may dispatch back through handleLoadVar as a shim.
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

    void VariableExecutor::handleLoadVarCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        auto* slot = state.cachedGlobalSlot;
        // Defensive: environment teardown can leave a CACHED opcode pointing
        // at a stale slot. Revert to the generic path and let handleLoadVar
        // re-resolve (or raise the canonical "Variable not found" error).
        if (!slot)
        {
            context.getMutableInstructionAt(context.instructionPointer).opcode
                = bytecode::OpCode::LOAD_VAR;
            handleLoadVar(instr);
            return;
        }
        context.stackManager->push(slot->getValue());
    }

    void VariableExecutor::handleStoreVarCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        auto* slot = state.cachedGlobalSlot;
        if (!slot)
        {
            context.getMutableInstructionAt(context.instructionPointer).opcode
                = bytecode::OpCode::STORE_VAR;
            handleStoreVar(instr);
            return;
        }
        if (instr.numOperands() == 0)
        {
            throw errors::RuntimeException("STORE_VAR_CACHED requires operand");
        }
        value::Value val = context.stackManager->pop();
        if (slot->isFinal())
        {
            const std::string& varName =
                context.program->getConstantPool().getString(instr.inlineOperands[0]);
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Cannot assign to final variable '" + varName + "'");
        }
        slot->setValue(val);
        context.stackManager->push(val);
    }
}
