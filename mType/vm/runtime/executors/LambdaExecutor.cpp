#include "LambdaExecutor.hpp"
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

    void LambdaExecutor::handleLambdaInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        // Pop argument count
        size_t argCount = instr.operands[0];

        // Pop arguments into a small-buffer-optimized scratch buffer
        // (MYT-196: avoids per-call heap allocation).
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // Pop lambda value from stack
        value::Value lambdaVal = context.stackManager->pop();
        auto lambda = value::asLambda(lambdaVal);

        size_t lambdaStart = lambda->instructionPointer;
        size_t paramCount = lambda->parameterCount;

        // Validate argument count
        if (args.size() != paramCount) {
            throw errors::RuntimeException("Lambda expects " + std::to_string(paramCount) +
                                         " arguments but got " + std::to_string(args.size()));
        }

        // Create call frame
        CallFrame frame;
        frame.returnAddress = context.instructionPointer;  // Return to next instruction
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        // Use the lambda's unique function name (e.g., __lambda_0) for metadata/exception table lookup.
        // MYT-197: 4-byte handle copy on the hot path. Falls back to a
        // program-interned "<lambda>" / "Class::<lambda>" display name only
        // when the lambda was built without going through handleLambda.
        if (lambda->functionName != bytecode::INVALID_FN_HANDLE) {
            frame.functionName = lambda->functionName;
        } else {
            std::string fallback = lambda->creatingClassName.empty()
                ? "<lambda>"
                : lambda->creatingClassName + "::<lambda>";
            frame.functionName = context.program->internFrameName(fallback);
        }
        frame.thisInstance = lambda->capturedThis;  // Restore captured 'this'
        frame.definingClassName = lambda->creatingClassName;  // Set creating class for access control
        frame.originatingLambda = lambda;  // Store lambda reference for variable access

        context.pushCallFrame(frame);

        // Notify debugger of lambda entry
        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            const std::string& frameNameStr = context.program->getFrameName(frame.functionName);
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
            } else {
                // Fallback: use lambda start location if current instruction has no location
                auto lambdaStartLoc = context.program->getSourceLocation(lambdaStart);
                if (lambdaStartLoc) {
                    errors::SourceLocation errorsLoc(lambdaStartLoc->filename, lambdaStartLoc->line, lambdaStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errors::SourceLocation());
                }
            }
        }

        // Create a SharedStackFrame for this lambda invocation to support nested closures
        // Link it to the parent frame so nested lambdas can access parent variables
        auto newSharedFrame = std::make_shared<SharedStackFrame>();
        newSharedFrame->parentFrame = lambda->capturedFrame;  // Link to parent
        if (!context.callStack.empty()) {
            context.callStack.back().sharedFrame = newSharedFrame;
        }

        // Push arguments onto stack (they become local variables at indices 0, 1, 2, ...)
        // Also register them by name in SharedStackFrame so nested lambdas can capture them
        for (size_t i = 0; i < args.size(); ++i) {
            context.stackManager->push(args[i]);

            // Register parameter by name in SharedStackFrame
            if (i < lambda->parameterNames.size()) {
                std::string paramName = lambda->parameterNames[i];
                if (!paramName.empty()) {
                    newSharedFrame->setLocal(paramName, i, args[i]);
                }
            }
        }

        // Get lambda metadata for parameter type information and local slot count.
        // MYT-197: O(1) handle lookup replaces the std::string hashmap probe.
        auto* lambdaMetadata = context.program->getFunctionMeta(lambda->functionName);

        // Auto-box primitive arguments if lambda expects boxed types
        // (mirrors ObjectExecutor::invokeLambdaMethod behavior)
        if (lambdaMetadata) {
            size_t stackBase = context.callStack.back().localBase;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i >= lambdaMetadata->parameterTypes.size()) break;
                std::string expectedType = lambdaMetadata->parameterTypes[i];
                value::Value& argValue = (*context.stackManager)[stackBase + i];

                bool needsBoxing = false;
                std::string boxClassName;

                if (expectedType == "Int" && value::isInt(argValue)) {
                    needsBoxing = true; boxClassName = "Int";
                } else if (expectedType == "Float" && (value::isFloat(argValue) || value::isInt(argValue))) {
                    needsBoxing = true; boxClassName = "Float";
                } else if (expectedType == "Bool" && value::isBool(argValue)) {
                    needsBoxing = true; boxClassName = "Bool";
                } else if (expectedType == "String" && value::isString(argValue)) {
                    needsBoxing = true; boxClassName = "String";
                }

                if (needsBoxing) {
                    auto classDef = context.environment->findClass(boxClassName);
                    if (classDef) {
                        if (classDef->isValueClass()) {
                            auto valueObj = std::make_shared<value::ValueObject>(classDef);
                            valueObj->setField("value", argValue);
                            argValue = value::Value(valueObj);
                        } else {
                            std::unordered_map<std::string, std::string> emptyBindings;
                            auto boxedInstance = value::ObjectInstancePool::getInstance().acquire(classDef, emptyBindings);
                            boxedInstance->setField("value", argValue);
                            argValue = value::Value(boxedInstance);
                        }
                        // Also update SharedStackFrame
                        if (i < lambda->parameterNames.size() && !lambda->parameterNames[i].empty()) {
                            newSharedFrame->setLocal(lambda->parameterNames[i], i, argValue);
                        }
                    }
                }
            }
        }

        // Push captured variables onto stack (they become local variables after the parameters)
        // Read current values from shared frame (reference capture semantics)
        // IMPORTANT: Do NOT register them in the new SharedStackFrame - they should be accessed
        // through the parent chain to ensure we always read the latest values
        size_t capturedCount = 0;
        if (lambda->capturedFrame) {
            for (size_t i = 0; i < lambda->capturedSlots.size(); ++i) {
                size_t slot = lambda->capturedSlots[i];

                // Always use slot-based lookup to avoid name collisions
                // This allows multiple variables with the same name to coexist
                value::Value capturedValue = lambda->capturedFrame->getLocal(slot);

                context.stackManager->push(capturedValue);
                capturedCount++;
            }
        }

        // Reserve additional local variable slots if needed (for local variables like return value temporaries)
        if (lambdaMetadata) {
            size_t pushedSlots = args.size() + capturedCount;
            if (lambdaMetadata->localCount > pushedSlots) {
                size_t additionalLocals = lambdaMetadata->localCount - pushedSlots;
                for (size_t i = 0; i < additionalLocals; ++i) {
                    context.stackManager->push(std::monostate{});  // Sentinel for uninitialized slot
                }
            }
        }

        // Jump to lambda start (subtract 1 because the VM loop will increment after this)
        context.instructionPointer = lambdaStart - 1;
    }
}
