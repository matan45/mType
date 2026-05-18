#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
#include "../context/SharedStackFramePool.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../environment/registry/VariableDefinition.hpp"
#include "../../../constants/SecurityConstants.hpp"
#include "../utils/ErrorLocationHelper.hpp"

namespace vm::runtime
{
    /**
     * Executes variable-related opcodes
     * Handles LOAD_VAR, STORE_VAR, DECLARE_VAR, LOAD_LOCAL, STORE_LOCAL
     * Manages global variables, local variables, and lambda closure mechanics
     *
     * MYT-319: LOAD_LOCAL / STORE_LOCAL hot paths and their type-quickened
     * variants (MYT-199) are inlined in the header so MSVC v145 (no /GL)
     * can inline them through the dispatch switch's unique_ptr deref.
     * Global-variable handling (LOAD_VAR / STORE_VAR / DECLARE_VAR) plus the
     * instance/static-field fallback helpers stay in the .cpp because they
     * pull in ObjectInstance/ClassDefinition + ValueTypeUtils.
     */
    class VariableExecutor
    {
    public:
        VariableExecutor(ExecutionContext& ctx,
                         std::shared_ptr<environment::Environment> env)
            : context(ctx), environment(std::move(env)) {}
        ~VariableExecutor() = default;

        // Global variable operations — out-of-line: need ObjectInstance/ClassDefinition.
        void handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-204: LOAD_VAR_CACHED / STORE_VAR_CACHED fast paths. After the
        // generic LOAD_VAR / STORE_VAR site successfully resolves a global,
        // the opcode is rewritten in place and the side table snapshots the
        // VariableDefinition pointer. The cached executors dereference the
        // slot directly — no constant-pool string fetch, no Environment
        // hashmap probe. Defensive null guard reverts to the generic path
        // for environment-teardown edge cases.
        inline void handleLoadVarCached(
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

        inline void handleStoreVarCached(
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

        // Local variable operations (stack-based)
        inline void handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr)
        {
            // MYT-318: operand-count contract enforced by program-load validator.
            loadLocalSlot(instr.inlineOperands[0]);
        }

        inline void handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr)
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

            // Check if we're in a lambda context and this is a captured variable.
            // If so, update it through the SharedStackFrame parent chain for
            // reference capture. Phase 2b: read originatingLambda via .get() —
            // copying the shared_ptr fires an atomic refcount op that the hot
            // lambda LOAD/STORE path can't afford.
            BytecodeLambda* lambda = nullptr;
            if (!context.callStack.empty())
            {
                lambda = context.callStack.back().originatingLambda.get();
            }
            if (lambda)
            {
                size_t paramCount = lambda->parameterCount;
                size_t capturedCount = lambda->capturedSlots.size();

                if (slot >= paramCount && slot < paramCount + capturedCount)
                {
                    size_t capturedIndex = slot - paramCount;
                    if (capturedIndex < lambda->capturedNames.size())
                    {
                        const std::string& capturedVarName = lambda->capturedNames[capturedIndex];

                        if (!capturedVarName.empty() && lambda->capturedFrame)
                        {
                            lambda->capturedFrame->setLocalByName(capturedVarName, val);
                            context.stackManager->push(val);
                            return;
                        }
                    }
                }
            }

            // Normal local variable access - write to stack
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

            // Also update the SharedStackFrame (for closure capture reference semantics)
            if (!lambda && !context.callStack.empty())
            {
                if (context.callStack.back().sharedFrame)
                {
                    context.callStack.back().sharedFrame->setLocal(slot, val);
                }
                else if (!varName.empty())
                {
                    context.callStack.back().sharedFrame = makePooledFrame();
                    auto sharedFrame = context.callStack.back().sharedFrame;
                    sharedFrame->setLocal(varName, slot, val);
                }
            }

            // Push value back for assignment expressions
            context.stackManager->push(val);

            // MYT-199: observe the stored type and, on a stable monomorphic
            // observation, promote this instruction to STORE_LOCAL_<TAG>.
            tryPromoteStoreLocal(val.tag());
        }

        // Slot-based fast-path entry used by MYT-202 fused opcode handlers.
        // Skips the Instruction::operands shim construction — the fused
        // dispatch already has the raw slot index from its own operands.
        // Semantics identical to handleLoadLocal / handleStoreLocal's
        // no-varName path; tryPromote{Load,Store}Local still correctly bails
        // when the current IP holds a fused opcode, not the generic form.
        inline void loadLocalSlot(size_t slot)
        {
            // SECURITY: cap the slot index symmetrically with storeLocalSlot.
            if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
            {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "LOAD_LOCAL slot index " + std::to_string(slot) +
                    " exceeds per-frame limit " +
                    std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
            }

            // Check if we're in a lambda context and this is a captured variable.
            // Phase 2b: read originatingLambda via .get() — the previous form
            // copied the shared_ptr per LOAD_LOCAL, firing an atomic refcount
            // op the lambda hot path can't absorb.
            BytecodeLambda* lambda = nullptr;
            if (!context.callStack.empty())
            {
                lambda = context.callStack.back().originatingLambda.get();
            }
            if (lambda)
            {
                size_t paramCount = lambda->parameterCount;
                size_t capturedCount = lambda->capturedSlots.size();

                if (slot >= paramCount && slot < paramCount + capturedCount)
                {
                    size_t capturedIndex = slot - paramCount;
                    if (capturedIndex < lambda->capturedNames.size())
                    {
                        size_t capturedSlot = lambda->capturedSlots[capturedIndex];

                        if (lambda->capturedFrame)
                        {
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
            if (!context.callStack.empty() && context.callStack.back().sharedFrame)
            {
                auto sharedFrame = context.callStack.back().sharedFrame;
                value::Value sharedVal = sharedFrame->getLocal(slot);

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

            value::Value val = (*context.stackManager)[stackPos];
            context.stackManager->push(val);

            // MYT-199: observe the loaded type and promote.
            tryPromoteLoadLocal(val.tag());
        }

        inline void storeLocalSlot(size_t slot)
        {
            // SECURITY: symmetric cap with handleStoreLocal.
            if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
            {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "STORE_LOCAL slot index " + std::to_string(slot) +
                    " exceeds per-frame limit " +
                    std::to_string(constants::security::MAX_LOCAL_STACK_PER_FRAME));
            }

            // MYT-318 fast path: when the frame is non-lambda AND the target slot
            // is *strictly below* TOS, copy TOS into the slot.
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
                    tryPromoteStoreLocal(tag);
                    return;
                }
            }

            value::Value val = context.stackManager->pop();

            // Lambda-captured path. Phase 2b: raw-pointer access avoids the
            // shared_ptr refcount atomic op on the lambda hot path.
            if (inLambda)
            {
                BytecodeLambda* lambda = context.callStack.back().originatingLambda.get();
                size_t paramCount = lambda->parameterCount;
                size_t capturedCount = lambda->capturedSlots.size();
                if (slot >= paramCount && slot < paramCount + capturedCount)
                {
                    size_t capturedIndex = slot - paramCount;
                    if (capturedIndex < lambda->capturedNames.size())
                    {
                        const std::string& capturedVarName = lambda->capturedNames[capturedIndex];
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

            // Mirror into an existing SharedStackFrame.
            if (!context.callStack.empty() && !context.callStack.back().originatingLambda
                && context.callStack.back().sharedFrame)
            {
                context.callStack.back().sharedFrame->setLocal(slot, val);
            }

            context.stackManager->push(val);
            tryPromoteStoreLocal(val.tag());
        }

        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. The generic
        // handlers above record their first observed ValueType and rewrite
        // the opcode in place to one of these variants. The specialized
        // handler guards on the expected tag; on a guard miss it demotes
        // back to the generic opcode with a sticky counter so the site
        // isn't re-promoted (mirrors MYT-173's CALL_METHOD_CACHED policy).
        inline void handleLoadLocalInt(const bytecode::BytecodeProgram::Instruction& instr) {
            handleLoadLocalSpecialized(instr, value::ValueType::INT);
        }

        inline void handleLoadLocalFloat(const bytecode::BytecodeProgram::Instruction& instr) {
            handleLoadLocalSpecialized(instr, value::ValueType::FLOAT);
        }

        inline void handleLoadLocalBool(const bytecode::BytecodeProgram::Instruction& instr) {
            handleLoadLocalSpecialized(instr, value::ValueType::BOOL);
        }

        inline void handleLoadLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr) {
            handleLoadLocalSpecialized(instr, value::ValueType::OBJECT);
        }

        inline void handleStoreLocalInt(const bytecode::BytecodeProgram::Instruction& instr) {
            handleStoreLocalSpecialized(instr, value::ValueType::INT);
        }

        inline void handleStoreLocalFloat(const bytecode::BytecodeProgram::Instruction& instr) {
            handleStoreLocalSpecialized(instr, value::ValueType::FLOAT);
        }

        inline void handleStoreLocalBool(const bytecode::BytecodeProgram::Instruction& instr) {
            handleStoreLocalSpecialized(instr, value::ValueType::BOOL);
        }

        inline void handleStoreLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr) {
            handleStoreLocalSpecialized(instr, value::ValueType::OBJECT);
        }

    private:
        ExecutionContext& context;
        std::shared_ptr<environment::Environment> environment;

        // Helper methods for handleLoadVar/StoreVar — out-of-line (need
        // ObjectInstance/ClassDefinition).
        bool tryLoadFromInstanceField(const std::string& varName);
        bool tryLoadFromStaticField(const std::string& varName);
        bool tryStoreToInstanceField(const std::string& varName, const value::Value& val);
        bool tryStoreToStaticField(const std::string& varName, const value::Value& val);
        void validateAndStoreGlobalVariable(const std::string& varName,
                                           const value::Value& val,
                                           std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);

        // MYT-204 promote helpers.
        void tryPromoteLoadVarCached(runtimeTypes::global::VariableDefinition* slot);
        void tryPromoteStoreVarCached(runtimeTypes::global::VariableDefinition* slot);

        // MYT-199 helpers — inlined here (small, called from inlined hot
        // paths above; out-of-lining them would defeat the migration).
        inline bool isCurrentFrameSimple() const
        {
            // Top-level script frame has no capture machinery — treat as simple.
            if (context.callStack.empty()) return true;
            const auto& frame = context.callStack.back();
            // Lambda bodies route LOAD_LOCAL through capturedFrame.
            if (frame.originatingLambda) return false;
            // sharedFrame with use_count == 1 is speculative bookkeeping.
            if (frame.sharedFrame && frame.sharedFrame.use_count() > 1) return false;
            return true;
        }

        inline void tryPromoteLoadLocal(value::ValueType observedTag)
        {
            const size_t ip = context.instructionPointer;

            // MYT-202: only promote if the IP actually holds a generic LOAD_LOCAL.
            if (context.getMutableInstructionAt(ip).opcode != bytecode::OpCode::LOAD_LOCAL)
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
                return;
            }

            auto& state = context.getOrCreateCachedState(ip);
            state.observedValueType = static_cast<uint8_t>(observedTag);

            auto& mut = context.getMutableInstructionAt(ip);
            mut.opcode = specialized;
        }

        inline void tryPromoteStoreLocal(value::ValueType observedTag)
        {
            const size_t ip = context.instructionPointer;

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

        inline void deoptLoadLocal(const bytecode::BytecodeProgram::Instruction& /*instr*/)
        {
            const size_t ip = context.instructionPointer;
            auto& mut = context.getMutableInstructionAt(ip);
            mut.opcode = bytecode::OpCode::LOAD_LOCAL;
            auto& state = context.getOrCreateCachedState(ip);
            if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
            handleLoadLocal(mut);
        }

        inline void deoptStoreLocal(const bytecode::BytecodeProgram::Instruction& /*instr*/)
        {
            const size_t ip = context.instructionPointer;
            auto& mut = context.getMutableInstructionAt(ip);
            mut.opcode = bytecode::OpCode::STORE_LOCAL;
            auto& state = context.getOrCreateCachedState(ip);
            if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
            handleStoreLocal(mut);
        }

        inline void handleLoadLocalSpecialized(
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
            // capture-aware read paths in handleLoadLocal.
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
                // Type guard miss — stickily demote.
                deoptLoadLocal(instr);
                return;
            }

            // Tag matched — MYT-199 fast path.
            context.stackManager->push(val);
        }

        inline void handleStoreLocalSpecialized(
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
            // handler to re-run on deopt.
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

            // Keep the varName resolution exactly as the generic handler.
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

            // SharedStackFrame propagation.
            if (!context.callStack.empty())
            {
                if (context.callStack.back().sharedFrame)
                {
                    context.callStack.back().sharedFrame->setLocal(slot, val);
                }
                else if (!varName.empty())
                {
                    context.callStack.back().sharedFrame = makePooledFrame();
                    context.callStack.back().sharedFrame->setLocal(varName, slot, val);
                }
            }

            context.stackManager->push(val);
        }
    };
}
