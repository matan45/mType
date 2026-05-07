#include "LambdaExecutor.hpp"
#include <cstddef>
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include <algorithm>
#include  <iostream>
namespace vm::runtime
{
    LambdaExecutor::LambdaExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void LambdaExecutor::handleLambda(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get lambda function start address, parameter count, capture count, and parent local count
        // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount, functionNameIdx,
        //                   captureSlot1, ..., captureSlotN,
        //                   paramNameIdx1, ..., paramNameIdxN,
        //                   capturedNameIdx1, ..., capturedNameIdxN,
        //                   parentNameIdx1, parentSlot1, ...]
        size_t lambdaStart = instr.operands[0];
        size_t paramCount = instr.operands[1];
        size_t captureCount = instr.operands[2];
        size_t parentLocalCount = instr.operands.size() > 3 ? instr.operands[3] : 0;
        size_t funcNameIdx = instr.operands[4];

        // Create bytecode lambda
        auto lambda = std::make_shared<BytecodeLambda>();
        lambda->instructionPointer = lambdaStart;
        lambda->parameterCount = paramCount;

        // Store function name handle for metadata lookup (MYT-197).
        // Intern the constant-pool string once at lambda creation; every
        // subsequent invocation just copies the 4-byte handle.
        lambda->functionName = context.program->internFrameName(
            context.program->getConstantPool().getString(funcNameIdx));

        // Extract parameter names for debugging
        size_t paramNamesStart = 5 + captureCount;  // Now starts at 5 (was 4)
        for (size_t i = 0; i < paramCount; ++i) {
            size_t nameIdx = instr.operands[paramNamesStart + i];
            std::string paramName = context.program->getConstantPool().getString(nameIdx);
            lambda->parameterNames.push_back(paramName);
        }

        // Extract captured variable names for debugging
        size_t capturedNamesStart = paramNamesStart + paramCount;
        for (size_t i = 0; i < captureCount; ++i) {
            size_t nameIdx = instr.operands[capturedNamesStart + i];
            std::string capturedName = context.program->getConstantPool().getString(nameIdx);
            lambda->capturedNames.push_back(capturedName);
        }

        // NOTE: We use reference capture - captured variables are accessed from shared frame at invocation time
        // This allows lambdas to see changes to captured variables

        // Capture class context for access modifier checks.
        // MYT-208: read class context from getThisInstanceRaw so stack-promoted
        // ctor frames still resolve creatingClassName. capturedThis stays
        // shared_ptr (lambdas outlive frames) and remains empty for
        // STACK_OBJECT-only frames — the escape analyzer rejects promotion
        // whenever a lambda captures the candidate, so this case shouldn't
        // arise in practice.
        if (!context.callStack.empty()) {
            auto& currentFrame = context.callStack.back();
            if (auto* rawThis = currentFrame.getThisInstanceRaw()) {
                // Instance method context - get class from 'this'
                lambda->creatingClassName = rawThis->getClassDefinition()->getName();
                lambda->capturedThis = currentFrame.thisInstance;  // empty for STACK_OBJECT
            } else if (!currentFrame.definingClassName.empty()) {
                // Static method context - MYT-197: prefer the frame's
                // definingClassName (already set at push time) over re-splitting
                // the qualified name. Keeps this path off std::string for
                // interned frame names.
                lambda->creatingClassName = currentFrame.definingClassName;
            } else {
                const std::string& funcName = context.frameName(currentFrame);
                size_t colonPos = funcName.find("::");
                if (colonPos != std::string::npos) {
                    lambda->creatingClassName = funcName.substr(0, colonPos);
                }
            }
        }

        // Get or create shared frame for the current function
        std::shared_ptr<SharedStackFrame> sharedFrame;
        if (!context.callStack.empty() && context.callStack.back().sharedFrame) {
            // Reuse existing shared frame
            sharedFrame = context.callStack.back().sharedFrame;
        } else {
            // Create new shared frame
            sharedFrame = std::make_shared<SharedStackFrame>();
            if (!context.callStack.empty()) {
                context.callStack.back().sharedFrame = sharedFrame;
            }
        }

        // Store captured variable slot indices and register them in current SharedStackFrame
        // Variables from the CURRENT scope need to be registered so the lambda can access them
        // Variables from PARENT scopes should already be in their respective frames
        // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount, functionNameIdx, captureSlot1, captureSlot2, ...]
        for (size_t i = 0; i < captureCount; ++i) {
            size_t varSlot = instr.operands[5 + i];  // Capture slots start at index 5 (after functionNameIdx)

            // Store the slot index for later access
            lambda->capturedSlots.push_back(varSlot);

            // Register captured variable in current SharedStackFrame WITH NAME for reference semantics
            // Always register the name-to-slot mapping so setLocalByName works for mutations
            size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
            size_t stackPos = frameBase + varSlot;

            std::string capturedName = lambda->capturedNames[i];
            if (stackPos < context.stackManager->size()) {
                value::Value val = (*context.stackManager)[stackPos];
                sharedFrame->setLocal(capturedName, varSlot, val);
            } else {
                // Variable slot not yet on the stack — register with monostate
                // so the name-to-slot mapping exists for later setLocalByName calls
                sharedFrame->setLocal(capturedName, varSlot, std::monostate{});
            }
        }

        // Store the shared frame reference in the lambda
        lambda->capturedFrame = sharedFrame;

        // Push lambda value onto stack
        context.stackManager->push(lambda);
    }

    // MYT-B1: handleLambdaInvoke removed — the LAMBDA_INVOKE opcode was
    // never emitted by the compiler, so this 160+-line dispatcher was
    // unreachable. Lambda invocation is handled by CALL_METHOD with the
    // lambda value as the receiver, routed through ObjectExecutor.
}
