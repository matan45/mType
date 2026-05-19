#include "ObjectInstanceHelper.hpp"
#include "../VirtualMachine.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"

namespace vm::runtime
{
    void ObjectInstanceHelper::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        // Operand[0] = classNameIndex, Operand[1] = argCount.
        if (instr.numOperands() < 2) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames).
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        // Build a Value preserving the receiver's storage tag so the super
        // ctor frame inherits stack-promoted lifetime when applicable.
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        // IMPORTANT: Use the current class name from the operand, NOT the instance's class.
        // The instance might be a subclass (e.g., Dog), but we're in the Mammal constructor
        // and need to call Mammal's parent (Animal), not Dog's parent (Mammal).
        auto classDef = environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        std::string parentClassName = classDef->getParentClassName();

        // Strip generic type arguments, e.g., "BaseContainer<T>" -> "BaseContainer".
        std::string baseParentClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseParentClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = environment->getClassRegistry()->findClass(baseParentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + baseParentClassName);
        }

        auto parentConstructor = parentClass->findConstructorByTypes(args.span());
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + baseParentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        std::string typeSignature = parentConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? baseParentClassName + "::<init>"
            : baseParentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from the current frame's program index so library-resident
        // execution finds the right program; hardcoded 0 would lose library context.
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (!funcMetadata) {
            const auto& loaded = vm->getLoadedPrograms();
            for (size_t i = 0; i < loaded.size(); ++i) {
                auto libFunc = loaded[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = loaded[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            // MYT-208: push receiver Value preserving the storage tag.
            context.stackManager->push(thisValue);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: intern on the target program. Constructor qualified names
            // are pre-registered (see registerFunction), so this is a hashmap hit
            // after first use.
            frame.functionName = targetProgram->internFrameName(constructorName);
            // MYT-208: tag-branch `this` ownership for the super ctor frame.
            if (value::isStackObject(thisValue))
            {
                frame.thisInstanceRaw = instanceRaw;
            }
            else
            {
                frame.thisInstance = currentFrame.thisInstance;
            }
            frame.definingClassName = baseParentClassName;
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (ctorStartLoc) {
                        errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent constructor '" + constructorName + "' has no bytecode.");
        }
    }

    void ObjectInstanceHelper::handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        // Operand[0] = classNameIndex, Operand[1] = argCount.
        if (instr.numOperands() < 2) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames).
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        auto classDef = environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        auto targetConstructor = classDef->findConstructorByTypes(args.span());
        if (!targetConstructor) {
            throw errors::RuntimeException("No constructor found in class " + currentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        std::string typeSignature = targetConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? currentClassName + "::<init>"
            : currentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (!funcMetadata) {
            const auto& loaded = vm->getLoadedPrograms();
            for (size_t i = 0; i < loaded.size(); ++i) {
                auto libFunc = loaded[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = loaded[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            // MYT-208: push receiver Value preserving the storage tag.
            context.stackManager->push(thisValue);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: intern on the target program.
            frame.functionName = targetProgram->internFrameName(constructorName);
            // MYT-208: tag-branch `this` ownership for the delegated ctor frame.
            if (value::isStackObject(thisValue))
            {
                frame.thisInstanceRaw = instanceRaw;
            }
            else
            {
                frame.thisInstance = currentFrame.thisInstance;
            }
            frame.definingClassName = currentClassName;
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (ctorStartLoc) {
                        errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Constructor '" + constructorName + "' has no bytecode.");
        }
    }
}
