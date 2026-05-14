#include "FunctionExecutor.hpp"
#include <cstddef>
#include "../../../errors/SourceLocation.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../environment/NativeContext.hpp"
#include "../VirtualMachine.hpp"
#include "../../jit/JitCodeCache.hpp"
#include "../../jit/JitContext.hpp"
#include <algorithm>
#include <iostream>
namespace vm::runtime
{
    FunctionExecutor::FunctionExecutor(ExecutionContext& ctx)
        : context(ctx)
    {
    }

    void FunctionExecutor::handleCall(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Get function name from constant pool
        std::string functionName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order) into a small-buffer-optimized
        // scratch buffer — MYT-196.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        // Check inline cache first, then fall back to full resolution.
        // MYT-201: CALL IC state lives in the per-IP side table. The first
        // dispatch at this IP has no entry; subsequent ones hit the cache.
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = nullptr;
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        const size_t callIp = context.instructionPointer;
        if (auto* cached = context.findCachedState(callIp))
        {
            if (cached->cachedFuncMetadata)
            {
                // Cache hit — skip native check and hash lookup
                funcMetadata = cached->cachedFuncMetadata;
                targetProgram = cached->cachedProgram;
                targetProgramIndex = cached->cachedProgramIndex;
            }
            else if (cached->cachedNativeFunc)
            {
                // FFI cache hit — skip both unordered_map::find calls into the
                // native registry. The slot is populated below on first dispatch.
                vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);
                environment::NativeContext nativeCtx{ context.environment, context.vm->shared_from_this() };
                value::Value result = cached->cachedNativeFunc(nativeCtx, args.span());
                vm::profiler::ProfilerHookHelper::onFunctionExit(functionName);
                context.stackManager->push(result);
                return;
            }
        }

        if (!funcMetadata) {
            // Cache miss — check native first, then bytecode lookup
            auto nativeRegistry = context.environment->getNativeRegistry();
            if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
            {
                auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
                if (nativeFunc)
                {
                    vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);

                    // Populate FFI cache for subsequent calls
                    auto& newCached = context.getOrCreateCachedState(callIp);
                    newCached.cachedNativeFunc = nativeFunc;

                    environment::NativeContext nativeCtx{ context.environment, context.vm->shared_from_this() };
                    value::Value result = nativeFunc(nativeCtx, args.span());

                    vm::profiler::ProfilerHookHelper::onFunctionExit(functionName);

                    context.stackManager->push(result);
                    return;
                }
            }

            funcMetadata = context.program->getFunction(functionName);

            if (!funcMetadata && context.loadedPrograms) {
                for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                    auto libFunc = (*context.loadedPrograms)[i]->getFunction(functionName);
                    if (libFunc) {
                        funcMetadata = libFunc;
                        targetProgramIndex = i;
                        targetProgram = (*context.loadedPrograms)[i];
                        break;
                    }
                }
            }

            // Populate cache for subsequent calls. Only allocate the side-
            // table entry on first successful resolve — keeps the generic
            // path allocation-free for native-only and missing-function sites.
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
            // Convert lambda arguments to interface implementations if needed.
            // Skip for primitive-only parameter lists (no conversion possible).
            if (!funcMetadata->allPrimitiveParams)
                convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

            // Create call frame
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase; // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size(); // Locals start after arguments (which are now popped)
            // MYT-197: intern on the target program so the handle belongs to
            // whatever program owns the function (cross-library calls).
            frame.functionName = targetProgram->internFrameName(functionName);
            frame.thisInstance = nullptr;
            frame.programIndex = targetProgramIndex;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);

            // Notify debugger of function entry
            if (debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc)
                {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(functionName, errorsLoc);
                }
                else
                {
                    // Fallback: use function start location if current instruction has no location
                    auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (funcStartLoc)
                    {
                        errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                         funcStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(functionName, errorsLoc);
                    }
                    else
                    {
                        debugger::DebugHookHelper::enterFunctionHook(functionName, errors::SourceLocation());
                    }
                }
            }

            // Push arguments onto operand stack as locals (they will be at frameBase + slot)
            for (size_t i = 0; i < argCount; ++i)
            {
                context.stackManager->push(args[i]);
            }

            // Reserve and initialize remaining local variable slots (beyond parameters)
            // to prevent showing uninitialized variables in debugger
            // All non-parameter slots are initialized to null until explicitly assigned by STORE_LOCAL
            for (size_t i = argCount; i < funcMetadata->localCount; ++i)
            {
                context.stackManager->push(std::monostate{});
            }

            // Jump to function start
            context.instructionPointer = funcMetadata->startOffset - 1; // -1 because loop will increment
            return;
        }

        throw errors::RuntimeException("Function not found: " + functionName);
    }

    void FunctionExecutor::handleCallFast(const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t funcIndex = instr.inlineOperands[0];
        size_t argCount = instr.inlineOperands[1];

        const auto* funcMetadata = context.program->getFunctionByIndex(funcIndex);
        if (!funcMetadata) {
            throw errors::RuntimeException("CALL_FAST: invalid function index " + std::to_string(funcIndex));
        }

        size_t frameBase = context.stackManager->size() - argCount;

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        // Skip for primitive-only parameter lists (no conversion possible).
        if (!funcMetadata->allPrimitiveParams)
            convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = context.stackManager->size();
        // MYT-197: intern once. mangledName is already registered with the
        // program (see registerFunction), so this lookup is a hashmap hit
        // after the first call site — no string rebuilding per call.
        frame.functionName = context.program->internFrameName(
            funcMetadata->mangledName.empty() ? funcMetadata->name : funcMetadata->mangledName);
        frame.thisInstance = nullptr;
        frame.programIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;

        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        // Match the frame name (mangled when overloadable) so the profiler's
        // entry/exit pair balances against ControlFlowExecutor's RETURN, which
        // reports the frame's interned name on exit.
        vm::profiler::ProfilerHookHelper::onFunctionEntry(
            funcMetadata->mangledName.empty() ? funcMetadata->name : funcMetadata->mangledName);

        if (debugger::DebugHookHelper::isDebuggingEnabled())
        {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc)
            {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errorsLoc);
            }
            else
            {
                auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                if (funcStartLoc)
                {
                    errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                     funcStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errorsLoc);
                }
                else
                {
                    debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errors::SourceLocation());
                }
            }
        }

        for (size_t i = 0; i < argCount; ++i)
        {
            context.stackManager->push(args[i]);
        }

        for (size_t i = argCount; i < funcMetadata->localCount; ++i)
        {
            context.stackManager->push(std::monostate{});
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void FunctionExecutor::handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t argCount = instr.inlineOperands[1];

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order) — MYT-196 small-buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        // Get qualified name from the constant pool — vector index, ~no cost.
        // Needed by frame setup (functionName intern, profiler/debugger hooks)
        // on both cache hit and miss; the substring parsing into className /
        // simpleMethodName is only needed on miss and lives below.
        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        // Try the per-IP IC FIRST — on hit we skip the qualified-name string
        // parsing, findClass, findStaticMethod, and validateStaticMethodAccess
        // entirely. The previous structure ran all of those on every call and
        // only used the cache to skip the type-signature build, leaving ~150ns
        // of per-call overhead even after warmup. Static call sites are
        // monomorphic by definition (the qualified name is baked into the
        // bytecode operand), so a hit is always semantically valid.
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
            // Slow path — first call (or post-deopt). Parse the qualified
            // name, resolve through the class registry, validate access, and
            // populate the IC for subsequent calls.
            size_t colonPos = qualifiedName.find("::");
            if (colonPos == std::string::npos)
            {
                throw errors::RuntimeException(
                    "Static method call requires qualified name (ClassName::methodName): " + qualifiedName);
            }

            std::string className = qualifiedName.substr(0, colonPos);
            std::string methodName = qualifiedName.substr(colonPos + 2);

            // Extract simple method name from potentially mangled name
            // methodName could be: "max/int,int$static", "max$static", or just "max"
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

            // Get class definition
            auto classRegistry = context.environment->getClassRegistry();
            auto classDef = classRegistry->findClass(className);
            if (!classDef)
            {
                throw errors::RuntimeException("Class not found: " + className);
            }

            // Find static method in class (use simple name for ClassDefinition lookup)
            auto method = classDef->findStaticMethod(simpleMethodName, argCount);
            if (!method)
            {
                throw errors::RuntimeException("Static method not found: " + className + "::" + simpleMethodName +
                    " with " + std::to_string(argCount) + " arguments");
            }

            // Check access modifiers (use simple name)
            validateStaticMethodAccess(className, simpleMethodName, method->getAccessModifier());

            // MYT-197: $static suffix is baked into the constant pool at compile
            // time (see FunctionCallHelper::emitStaticMethodCall), so
            // qualifiedName already ends in $static — no per-call concat.
            const std::string& staticQualifiedName = qualifiedName;

            // Cache miss — full resolution with type-signature-based overload lookup

            // Build type signature from runtime argument types
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
                else if (value::isString(arg))
                {
                    typeSignature += "string";
                }
                else if (value::isInternedString(arg))
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

            // Try lookup with type signature first: ClassName::methodName/paramTypes$static
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

            if (!funcMetadata && context.loadedPrograms) {
                for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                    funcMetadata = (*context.loadedPrograms)[i]->getFunction(signedQualifiedName);
                    if (!funcMetadata) {
                        funcMetadata = (*context.loadedPrograms)[i]->getFunction(staticQualifiedName);
                    }
                    if (funcMetadata) {
                        targetProgramIndex = i;
                        targetProgram = (*context.loadedPrograms)[i];
                        break;
                    }
                }
            }

            // Populate cache for subsequent calls. Only allocate the side-
            // table entry on first successful resolve.
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
            // Convert lambda arguments to interface implementations if needed.
            // Skip for primitive-only parameter lists (no conversion possible).
            if (!funcMetadata->allPrimitiveParams)
                convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

            // Create call frame for static method
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase; // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size(); // Locals start after arguments (which are now popped)
            // MYT-197: intern on the target program so the handle belongs to
            // the program that actually owns the function.
            frame.functionName = targetProgram->internFrameName(qualifiedName);
            frame.thisInstance = nullptr; // No 'this' for static methods
            frame.programIndex = targetProgramIndex;

            // Switch to library program if function is from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            vm::profiler::ProfilerHookHelper::onFunctionEntry(qualifiedName);

            // Notify debugger of static method entry
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
                    // Fallback: use function start location if current instruction has no location
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

            // Push arguments onto stack as locals (slot 0, 1, 2, ...)
            for (size_t i = 0; i < argCount; ++i)
            {
                context.stackManager->push(args[i]);
            }

            // Reserve space for local variables (beyond parameters)
            // localCount includes parameters, so we need (localCount - argCount) additional slots
            if (funcMetadata->localCount > argCount)
            {
                size_t additionalLocals = funcMetadata->localCount - argCount;
                for (size_t i = 0; i < additionalLocals; ++i)
                {
                    context.stackManager->push(std::monostate{});  // Initialize with null
                }
            }

            // Jump to static method start
            context.instructionPointer = funcMetadata->startOffset - 1; // -1 because loop will increment
        }
        else
        {
            throw errors::RuntimeException(
                "Static method '" + qualifiedName +
                "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    void FunctionExecutor::convertLambdaArgumentsToInterfaces(
        std::span<value::Value> args,
        const std::vector<std::string>& parameterTypes
    )
    {
        // NOTE: Bytecode VM uses BytecodeLambda which are handled directly by ObjectExecutor::handleCallMethod
        // and don't need interface wrapping

        // Calculate parameter type offset
        // For instance methods, parameterTypes includes 'this' as the first element, but args doesn't
        // So if parameterTypes.size() > args.size(), we need to skip the first parameter type
        size_t paramOffset = 0;
        if (parameterTypes.size() > args.size()) {
            paramOffset = 1; // Skip 'this' parameter
        }

        for (size_t i = 0; i < args.size(); ++i)
        {
            size_t paramIndex = i + paramOffset;
            if (paramIndex >= parameterTypes.size()) break;

            const std::string& paramType = parameterTypes[paramIndex];
            value::Value& arg = args[i];

            // AUTO-UNBOXING: Convert wrapper objects to primitives (Int → int, Float → float, etc.)
            if (paramType == "int" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Int")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isInt(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "float" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Float")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isFloat(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "bool" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Bool")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isBool(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "string" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "String")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isString(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }

            // AUTO-BOXING: Convert primitives to wrapper objects (Int, Float, Bool, String)
            if (paramType == "Int" && value::isInt(arg))
            {
                // Auto-box int to Int
                auto intClass = context.environment->findClass("Int");
                if (intClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(intClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Float" && value::isFloat(arg))
            {
                // Auto-box float to Float
                auto floatClass = context.environment->findClass("Float");
                if (floatClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(floatClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Bool" && value::isBool(arg))
            {
                // Auto-box bool to Bool
                auto boolClass = context.environment->findClass("Bool");
                if (boolClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(boolClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "String" && value::isString(arg))
            {
                // Auto-box string to String
                auto stringClass = context.environment->findClass("String");
                if (stringClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(stringClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }

            // BytecodeLambda (bytecode VM) - no conversion needed
            // ObjectExecutor::handleCallMethod handles BytecodeLambda invocation directly
            if (value::isLambda(arg))
            {
                // No conversion needed - bytecode lambdas are invoked directly
                continue;
            }
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
            return; // Public methods are always accessible
        }

        // Get current execution context (the class we're executing from)
        std::string currentClassName;
        if (!context.callStack.empty())
        {
            // MYT-208: accept stack-promoted `this`.
            if (auto* rawThis = context.callStack.back().getThisInstanceRaw())
            {
                // Instance method context
                currentClassName = rawThis->getClassDefinition()->getName();
            }
            else if (!context.callStack.back().definingClassName.empty())
            {
                // Static method context - MYT-197: prefer frame.definingClassName
                // (already set at push time) over resolving + splitting the
                // interned function-name handle.
                currentClassName = context.callStack.back().definingClassName;
            }
            else
            {
                const std::string& funcName = context.frameName(context.callStack.back());
                size_t colonPos = funcName.find("::");
                if (colonPos != std::string::npos)
                {
                    currentClassName = funcName.substr(0, colonPos);
                }
            }
        }

        if (accessMod == ast::AccessModifier::PRIVATE)
        {
            // PRIVATE: Only accessible from same class
            if (currentClassName != className)
            {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;

                // Get current source location for error reporting
                errors::SourceLocation location(
                    context.currentSourceFile,
                    context.currentSourceLine,
                    1 // Column information not currently tracked
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
            // PROTECTED: Accessible from same class and subclasses
            if (currentClassName != className)
            {
                // Check if current class is a subclass of target class
                bool isSubclass = false;
                if (!currentClassName.empty())
                {
                    auto currentClass = context.environment->getClassRegistry()->findClass(currentClassName);
                    while (currentClass && currentClass->hasParentClass())
                    {
                        if (currentClass->getParentClassName() == className)
                        {
                            isSubclass = true;
                            break;
                        }
                        // Move to parent class
                        auto parentClass = context.environment->getClassRegistry()->findClass(
                            currentClass->getParentClassName());
                        currentClass = parentClass;
                    }
                }

                if (!isSubclass)
                {
                    std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;

                    // Get current source location for error reporting
                    errors::SourceLocation location(
                        context.currentSourceFile,
                        context.currentSourceLine,
                        1 // Column information not currently tracked
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
