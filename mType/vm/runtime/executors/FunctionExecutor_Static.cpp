#include "FunctionExecutor.hpp"
#include <cstddef>
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../environment/registry/ClassRegistry.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ValueShim.hpp"
#include "../VirtualMachine.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"

namespace vm::runtime
{
    void FunctionExecutor::handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t argCount = instr.inlineOperands[1];

        // frameBase captured BEFORE popping arguments.
        size_t frameBase = context.stackManager->size() - argCount;

        // MYT-196: small-buffer args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        // qualifiedName is needed by frame setup (intern + profiler/debugger
        // hooks) on both cache hit and miss; substring parsing into className
        // and simpleMethodName is only needed on miss.
        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        // Try the per-IP IC FIRST — on hit we skip the qualified-name string
        // parsing, findClass, findStaticMethod, and validateStaticMethodAccess
        // entirely. Static call sites are monomorphic by definition (the
        // qualified name is baked into the bytecode operand), so a hit is
        // always semantically valid.
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = nullptr;
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        const size_t callIp = context.instructionPointer;
        if (auto* cached = context.findCachedState(callIp))
        {
            if (cached->cachedFuncMetadata)
            {
                funcMetadata = cached->cachedFuncMetadata;
                targetProgram = cached->cachedProgram;
                targetProgramIndex = cached->cachedProgramIndex;
            }
        }

        if (!funcMetadata) {
            // Slow path — first call (or post-deopt). Parse the qualified name,
            // resolve through the class registry, validate access, and populate
            // the IC for subsequent calls.
            size_t colonPos = qualifiedName.find("::");
            if (colonPos == std::string::npos)
            {
                throw errors::RuntimeException(
                    "Static method call requires qualified name (ClassName::methodName): " + qualifiedName);
            }

            std::string className = qualifiedName.substr(0, colonPos);
            std::string methodName = qualifiedName.substr(colonPos + 2);

            // methodName can be "max/int,int$static", "max$static", or just "max".
            std::string simpleMethodName = methodName;

            size_t staticPos = simpleMethodName.find("$static");
            if (staticPos != std::string::npos)
            {
                simpleMethodName = simpleMethodName.substr(0, staticPos);
            }

            size_t slashPos = simpleMethodName.find('/');
            if (slashPos != std::string::npos)
            {
                simpleMethodName = simpleMethodName.substr(0, slashPos);
            }

            auto classRegistry = environment->getClassRegistry();
            auto classDef = classRegistry->findClass(className);
            if (!classDef)
            {
                throw errors::RuntimeException("Class not found: " + className);
            }

            auto method = classDef->findStaticMethod(simpleMethodName, argCount);
            if (!method)
            {
                throw errors::RuntimeException("Static method not found: " + className + "::" + simpleMethodName +
                    " with " + std::to_string(argCount) + " arguments");
            }

            validateStaticMethodAccess(className, simpleMethodName, method->getAccessModifier());

            // MYT-197: $static suffix is baked into the constant pool at compile
            // time (see FunctionCallHelper::emitStaticMethodCall), so
            // qualifiedName already ends in $static — no per-call concat.
            const std::string& staticQualifiedName = qualifiedName;

            // Build type signature from runtime argument types for overload lookup.
            std::string typeSignature;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) typeSignature += ",";
                const auto& arg = args[i];
                if (value::isInt(arg))
                {
                    typeSignature += "int";
                }
                else if (value::isFloat(arg))
                {
                    typeSignature += "float";
                }
                else if (value::isBool(arg))
                {
                    typeSignature += "bool";
                }
                // MYT-317: STRING_INLINE also classifies as "string".
                else if (value::isAnyString(arg))
                {
                    typeSignature += "string";
                }
                else if (value::isObject(arg))
                {
                    auto obj = value::asObject(arg);
                    typeSignature += obj->getTypeName();
                }
                else if (value::isNativeArray(arg))
                {
                    auto arr = value::asNativeArray(arg);
                    switch (arr->getElementType())
                    {
                        case value::ValueType::INT: typeSignature += "int[]"; break;
                        case value::ValueType::FLOAT: typeSignature += "float[]"; break;
                        case value::ValueType::BOOL: typeSignature += "bool[]"; break;
                        case value::ValueType::STRING: typeSignature += "string[]"; break;
                        case value::ValueType::OBJECT:
                            typeSignature += arr->getElementTypeName() + "[]";
                            break;
                        default: typeSignature += "any[]"; break;
                    }
                }
                else if (value::isLambda(arg))
                {
                    typeSignature += "function";
                }
                else if (value::isVoid(arg) || value::isNullType(arg))
                {
                    typeSignature += "null";
                }
                else
                {
                    typeSignature += "any";
                }
            }

            // Try ClassName::methodName/paramTypes$static first.
            std::string signedQualifiedName = className + "::" + simpleMethodName;
            if (!typeSignature.empty())
            {
                signedQualifiedName += "/" + typeSignature;
            }
            signedQualifiedName += "$static";

            funcMetadata = context.program->getFunction(signedQualifiedName);

            if (!funcMetadata)
            {
                funcMetadata = context.program->getFunction(staticQualifiedName);
            }

            if (!funcMetadata) {
                const auto& loaded = vm->getLoadedPrograms();
                for (size_t i = 0; i < loaded.size(); ++i) {
                    funcMetadata = loaded[i]->getFunction(signedQualifiedName);
                    if (!funcMetadata) {
                        funcMetadata = loaded[i]->getFunction(staticQualifiedName);
                    }
                    if (funcMetadata) {
                        targetProgramIndex = i;
                        targetProgram = loaded[i];
                        break;
                    }
                }
            }

            if (funcMetadata) {
                auto& cached = context.getOrCreateCachedState(callIp);
                cached.cachedFuncMetadata = funcMetadata;
                cached.cachedStartOffset = funcMetadata->startOffset;
                cached.cachedProgram = targetProgram;
                cached.cachedProgramIndex = targetProgramIndex;
            }
        }

        if (funcMetadata)
        {
            if (!funcMetadata->allPrimitiveParams)
                convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = context.stackManager->size();
            // MYT-197: intern on the target program so the handle belongs to
            // the program that actually owns the function.
            frame.functionName = targetProgram->internFrameName(qualifiedName);
            frame.thisInstance = nullptr;
            frame.programIndex = targetProgramIndex;

            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            vm::profiler::ProfilerHookHelper::onFunctionEntry(qualifiedName);

            if (debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc)
                {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                }
                else
                {
                    auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (funcStartLoc)
                    {
                        errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                         funcStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                    }
                    else
                    {
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errors::SourceLocation());
                    }
                }
            }

            for (size_t i = 0; i < argCount; ++i)
            {
                context.stackManager->push(args[i]);
            }

            // localCount includes parameters; reserve (localCount - argCount) extra slots.
            if (funcMetadata->localCount > argCount)
            {
                size_t additionalLocals = funcMetadata->localCount - argCount;
                for (size_t i = 0; i < additionalLocals; ++i)
                {
                    context.stackManager->push(std::monostate{});
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        }
        else
        {
            throw errors::RuntimeException(
                "Static method '" + qualifiedName +
                "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    void FunctionExecutor::validateStaticMethodAccess(
        const std::string& className,
        const std::string& methodName,
        ast::AccessModifier accessMod
    )
    {
        if (accessMod == ast::AccessModifier::PUBLIC)
        {
            return;
        }

        std::string currentClassName;
        if (!context.callStack.empty())
        {
            // MYT-208: accept stack-promoted `this`.
            if (auto* rawThis = context.callStack.back().getThisInstanceRaw())
            {
                currentClassName = rawThis->getClassDefinition()->getName();
            }
            else if (!context.callStack.back().definingClassName.empty())
            {
                // MYT-197: prefer frame.definingClassName (already set at push
                // time) over resolving + splitting the interned function-name handle.
                currentClassName = context.callStack.back().definingClassName;
            }
            else
            {
                const std::string& funcName = vm->frameName(context.callStack.back());
                size_t colonPos = funcName.find("::");
                if (colonPos != std::string::npos)
                {
                    currentClassName = funcName.substr(0, colonPos);
                }
            }
        }

        if (accessMod == ast::AccessModifier::PRIVATE)
        {
            if (currentClassName != className)
            {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;

                errors::SourceLocation location(
                    context.currentSourceFile,
                    context.currentSourceLine,
                    1
                );

                throw errors::AccessViolationException(
                    methodName,
                    "method",
                    ast::AccessModifier::PRIVATE,
                    className,
                    callingFrom,
                    location
                );
            }
        }
        else if (accessMod == ast::AccessModifier::PROTECTED)
        {
            if (currentClassName != className)
            {
                bool isSubclass = false;
                if (!currentClassName.empty())
                {
                    auto currentClass = environment->getClassRegistry()->findClass(currentClassName);
                    while (currentClass && currentClass->hasParentClass())
                    {
                        if (currentClass->getParentClassName() == className)
                        {
                            isSubclass = true;
                            break;
                        }
                        auto parentClass = environment->getClassRegistry()->findClass(
                            currentClass->getParentClassName());
                        currentClass = parentClass;
                    }
                }

                if (!isSubclass)
                {
                    std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;

                    errors::SourceLocation location(
                        context.currentSourceFile,
                        context.currentSourceLine,
                        1
                    );

                    throw errors::AccessViolationException(
                        methodName,
                        "method",
                        ast::AccessModifier::PROTECTED,
                        className,
                        callingFrom,
                        location
                    );
                }
            }
        }
    }
}
