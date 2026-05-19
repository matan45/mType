#include "ObjectInstanceHelper.hpp"
#include "../VirtualMachine.hpp"
#include "../../MethodSignature.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"

namespace vm::runtime
{
    void ObjectInstanceHelper::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, methodNameIndex, argCount)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = method name
        // operand[2] = argument count
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string& methodName = context.program->getConstantPool().getString(instr.inlineOperands[1]);
        size_t argCount = instr.inlineOperands[2];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_INVOKE can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        // IMPORTANT: Use currentClassName from operand, NOT instance->getClassDefinition().
        // This prevents infinite recursion in multi-level inheritance
        // (e.g., AdvancedService -> DerivedService -> BaseService).
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

        auto method = parentClass->findMethod(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Method not found in parent class " + baseParentClassName + ": " + methodName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Use MethodSignature to build the mangled name; eliminates manual
        // signature building and 'this' confusion for overloaded methods.
        auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
        std::string qualifiedName = signature.toMangledName(baseParentClassName, false);

        auto funcMetadata = context.program->getFunction(qualifiedName);
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (!funcMetadata) {
            const auto& loaded = vm->getLoadedPrograms();
            for (size_t i = 0; i < loaded.size(); ++i) {
                auto libFunc = loaded[i]->getFunction(qualifiedName);
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
            frame.functionName = targetProgram->internFrameName(qualifiedName);
            // MYT-208: tag-branch `this` ownership for the parent method frame.
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
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                } else {
                    auto methodStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (methodStartLoc) {
                        errors::SourceLocation errorsLoc(methodStartLoc->filename, methodStartLoc->line, methodStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent method '" + qualifiedName + "' has no bytecode.");
        }
    }

    void ObjectInstanceHelper::handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.inlineOperands[1]);

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_GET_FIELD can only be called from within an instance context");
        }

        auto* instance = context.callStack.back().getThisInstanceRaw();

        auto classDef = environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        // MYT-212: walk parent chain for the first ancestor that declares the
        // field, then read its OWN slot. The previous code called
        // instance->getFieldValue(name), which routes via the instance's
        // runtime class (Child) and so returned the most-derived (shadowed)
        // value — exactly what super.x is supposed to skip.
        auto parent = classDef->getParentClass();
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> owner;
        auto current = parent;
        int depth = 0;
        while (current && depth < 20) {
            if (current->getField(memberName)) {
                owner = current;
                break;
            }
            current = current->getParentClass();
            ++depth;
        }
        if (!owner) {
            throw errors::RuntimeException("Field '" + memberName + "' not found in parent class chain of " + currentClassName);
        }

        size_t slot = owner->getOwnFieldIndex(memberName);
        if (slot == SIZE_MAX) {
            throw errors::RuntimeException("Field '" + memberName + "' has no indexed slot in " + owner->getName());
        }

        context.stackManager->push(instance->getFieldByIndex(slot));
    }

    void ObjectInstanceHelper::handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.inlineOperands[1]);

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_SET_FIELD can only be called from within an instance context");
        }

        auto* instance = context.callStack.back().getThisInstanceRaw();

        auto classDef = environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        value::Value assignValue = context.stackManager->pop();

        // MYT-212: write to parent's OWN slot (see handleSuperGetField for rationale).
        auto parent = classDef->getParentClass();
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> owner;
        auto current = parent;
        int depth = 0;
        while (current && depth < 20) {
            if (current->getField(memberName)) {
                owner = current;
                break;
            }
            current = current->getParentClass();
            ++depth;
        }
        if (!owner) {
            throw errors::RuntimeException("Field '" + memberName + "' not found in parent class chain of " + currentClassName);
        }

        size_t slot = owner->getOwnFieldIndex(memberName);
        if (slot == SIZE_MAX) {
            throw errors::RuntimeException("Field '" + memberName + "' has no indexed slot in " + owner->getName());
        }

        instance->setFieldByIndex(slot, assignValue);
    }
}
