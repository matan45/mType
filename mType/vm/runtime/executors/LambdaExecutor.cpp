#include "LambdaExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include <algorithm>

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

        // Share the parent frame for late-bound variable access
        // This allows lambdas to access variables declared after lambda creation (forward references)
        if (!context.callStack.empty()) {
            // Get or create shared frame for current call frame
            if (!context.callStack.back().sharedFrame) {
                context.callStack.back().sharedFrame = std::make_shared<SharedStackFrame>();
                // Initialize with current local variables
                size_t localBase = context.callStack.back().localBase;
                for (size_t i = localBase; i < context.stackManager->size(); ++i) {
                    context.callStack.back().sharedFrame->setLocal(i - localBase, (*context.stackManager)[i]);
                }
            }
            lambda->parentFrame = context.callStack.back().sharedFrame;

            // Populate the name->slot mapping from the LAMBDA instruction operands
            // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount, functionNameIdx,
            //                   captureSlot1, ..., captureSlotN,
            //                   paramNameIdx1, ..., paramNameIdxN,
            //                   capturedNameIdx1, ..., capturedNameIdxN,
            //                   parentNameIdx1, parentSlot1, ...]
            // Note: We ADD to the existing mapping, not replace it, so later lambdas
            // in the same scope can see earlier ones (for forward references)
            size_t mappingStart = 5 + captureCount + paramCount + captureCount;  // Skip header, funcNameIdx, capture slots, param names, and captured names
            for (size_t i = 0; i < parentLocalCount; ++i) {
                size_t nameIdx = instr.operands[mappingStart + i * 2];
                size_t slot = instr.operands[mappingStart + i * 2 + 1];
                std::string varName = context.program->getConstantPool().getString(nameIdx);
                // Only add if not already present (don't overwrite)
                if (lambda->parentFrame->nameToSlot.find(varName) == lambda->parentFrame->nameToSlot.end()) {
                    lambda->parentFrame->nameToSlot[varName] = slot;
                }
            }
        } else {
            // Global scope - create a new shared frame
            lambda->parentFrame = std::make_shared<SharedStackFrame>();
            for (size_t i = 0; i < context.stackManager->size(); ++i) {
                lambda->parentFrame->setLocal(i, (*context.stackManager)[i]);
            }
        }

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

        // Capture VALUES of variables from current stack frame (snapshot at creation time)
        // This ensures immutable capture semantics
        // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount, functionNameIdx, captureSlot1, captureSlot2, ...]
        for (size_t i = 0; i < captureCount; ++i) {
            size_t varSlot = instr.operands[5 + i];  // Capture slots start at index 5 (after functionNameIdx)

            // Read the current value from the parent frame's stack
            value::Value capturedValue = std::monostate{};
            if (!context.callStack.empty()) {
                size_t parentLocalBase = context.callStack.back().localBase;
                size_t stackPos = parentLocalBase + varSlot;
                if (stackPos < context.stackManager->size()) {
                    capturedValue = (*context.stackManager)[stackPos];
                }
            }

            lambda->capturedValues.push_back(capturedValue);
        }

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

        // Push arguments onto stack (they become local variables at indices 0, 1, 2, ...)
        for (size_t i = 0; i < args.size(); ++i) {
            context.stackManager->push(args[i]);
        }

        // Push captured variables onto stack (they become local variables after the parameters)
        // Use snapshot values (immutable capture semantics)
        for (const auto& capturedValue : lambda->capturedValues) {
            context.stackManager->push(capturedValue);
        }

        // Jump to lambda start (subtract 1 because the VM loop will increment after this)
        context.instructionPointer = lambdaStart - 1;
    }
}
