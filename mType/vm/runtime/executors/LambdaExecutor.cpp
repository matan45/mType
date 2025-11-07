#include "LambdaExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
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

        // Store function name for metadata lookup
        lambda->functionName = context.program->getConstantPool().getString(funcNameIdx);

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

        // Capture class context for access modifier checks
        if (!context.callStack.empty()) {
            if (context.callStack.back().thisInstance) {
                // Instance method context - get class from 'this'
                lambda->creatingClassName = context.callStack.back().thisInstance->getClassDefinition()->getName();
                lambda->capturedThis = context.callStack.back().thisInstance;
            } else {
                // Static method context - extract class name from function name (ClassName::methodName)
                const std::string& funcName = context.callStack.back().functionName;
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

            // Register captured variable in current SharedStackFrame if not already there
            if (i < lambda->capturedNames.size() && !lambda->capturedNames[i].empty()) {
                std::string varName = lambda->capturedNames[i];

                // Check if this variable is already registered in the parent chain
                value::Value existingVal = sharedFrame->getLocalByName(varName);

                // If not found in parent chain, it's a variable from current scope - register it
                if (std::holds_alternative<std::monostate>(existingVal)) {
                    // Read current value from stack
                    size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
                    size_t stackPos = frameBase + varSlot;

                    if (stackPos < context.stackManager->size()) {
                        value::Value val = (*context.stackManager)[stackPos];
                        sharedFrame->setLocal(varName, varSlot, val);
                    }
                }
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

        // Pop arguments from stack
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        // Pop lambda value from stack
        value::Value lambdaVal = context.stackManager->pop();
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
        frame.returnAddress = context.instructionPointer;  // Return to next instruction
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        // Preserve class context for access validation: ClassName::<lambda> or just <lambda>
        frame.functionName = lambda->creatingClassName.empty() ?
            "<lambda>" :
            lambda->creatingClassName + "::<lambda>";
        frame.thisInstance = lambda->capturedThis;  // Restore captured 'this'
        frame.definingClassName = lambda->creatingClassName;  // Set creating class for access control
        frame.originatingLambda = lambda;  // Store lambda reference for variable access

        context.pushCallFrame(frame);

        // Notify debugger of lambda entry
        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(frame.functionName, errorsLoc);
            } else {
                // Fallback: use lambda start location if current instruction has no location
                auto lambdaStartLoc = context.program->getSourceLocation(lambdaStart);
                if (lambdaStartLoc) {
                    errors::SourceLocation errorsLoc(lambdaStartLoc->filename, lambdaStartLoc->line, lambdaStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(frame.functionName, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(frame.functionName, errors::SourceLocation());
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

        // Push captured variables onto stack (they become local variables after the parameters)
        // Read current values from shared frame (reference capture semantics)
        // IMPORTANT: Do NOT register them in the new SharedStackFrame - they should be accessed
        // through the parent chain to ensure we always read the latest values
        if (lambda->capturedFrame) {
            for (size_t i = 0; i < lambda->capturedSlots.size(); ++i) {
                size_t slot = lambda->capturedSlots[i];
                std::string varName = (i < lambda->capturedNames.size()) ? lambda->capturedNames[i] : "";

                value::Value capturedValue;
                if (!varName.empty()) {
                    // Look up by name through the parent chain
                    capturedValue = lambda->capturedFrame->getLocalByName(varName);
                    if (std::holds_alternative<std::monostate>(capturedValue)) {
                        // Fallback to slot-based lookup if name lookup failed
                        capturedValue = lambda->capturedFrame->getLocal(slot);
                    }
                } else {
                    // No name available, use slot-based lookup
                    capturedValue = lambda->capturedFrame->getLocal(slot);
                }

                context.stackManager->push(capturedValue);
            }
        }

        // Jump to lambda start (subtract 1 because the VM loop will increment after this)
        context.instructionPointer = lambdaStart - 1;
    }
}
