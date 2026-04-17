#include "FunctionExecutor.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../value/NativeArray.hpp"
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
        std::string functionName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i)
        {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        // Check inline cache first, then fall back to full resolution
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = instr.cachedFuncMetadata;
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (funcMetadata) {
            // Cache hit — skip native check and hash lookup
            targetProgram = instr.cachedProgram;
            targetProgramIndex = instr.cachedProgramIndex;
        } else {
            // Cache miss — check native first, then bytecode lookup
            auto nativeRegistry = context.environment->getNativeRegistry();
            if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
            {
                auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
                if (nativeFunc)
                {
                    vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);

                    value::Value result = nativeFunc(args);

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

            // Populate cache for subsequent calls
            if (funcMetadata) {
                instr.cachedFuncMetadata = funcMetadata;
                instr.cachedStartOffset = funcMetadata->startOffset;
                instr.cachedProgram = targetProgram;
                instr.cachedProgramIndex = targetProgramIndex;
            }
        }

        if (funcMetadata)
        {
            // Convert lambda arguments to interface implementations if needed
            convertLambdaArgumentsToInterfaces(args, funcMetadata->parameterTypes);

            // Create call frame
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase; // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size(); // Locals start after arguments (which are now popped)
            frame.functionName = functionName;
            frame.thisInstance = nullptr;
            frame.programIndex = targetProgramIndex;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(frame);
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
        size_t funcIndex = instr.operands[0];
        size_t argCount = instr.operands[1];

        const auto* funcMetadata = context.program->getFunctionByIndex(funcIndex);
        if (!funcMetadata) {
            throw errors::RuntimeException("CALL_FAST: invalid function index " + std::to_string(funcIndex));
        }

        size_t frameBase = context.stackManager->size() - argCount;

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i)
        {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        convertLambdaArgumentsToInterfaces(args, funcMetadata->parameterTypes);

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = context.stackManager->size();
        frame.functionName = funcMetadata->mangledName.empty() ? funcMetadata->name : funcMetadata->mangledName;
        frame.thisInstance = nullptr;
        frame.programIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;

        context.pushCallFrame(frame);
        context.stats.functionCalls++;

        vm::profiler::ProfilerHookHelper::onFunctionEntry(funcMetadata->name);

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
        // Get static method name from constant pool (should be fully qualified: ClassName::methodName)
        std::string qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Parse qualified name: ClassName::methodName
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

        // Remove $static suffix if present
        size_t staticPos = simpleMethodName.find("$static");
        if (staticPos != std::string::npos)
        {
            simpleMethodName = simpleMethodName.substr(0, staticPos);
        }

        // Remove signature suffix if present
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

        // Calculate frameBase BEFORE popping arguments
        size_t frameBase = context.stackManager->size() - argCount;

        // Pop arguments from stack (in reverse order)
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i)
        {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        // Build base qualified name with $static suffix (needed for frame.functionName on all paths)
        std::string staticQualifiedName = qualifiedName;
        if (staticQualifiedName.find("$static") == std::string::npos)
        {
            staticQualifiedName += "$static";
        }

        // Check inline cache first, then fall back to full resolution
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = instr.cachedFuncMetadata;
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (funcMetadata) {
            // Cache hit — skip type-signature building and hash lookups
            targetProgram = instr.cachedProgram;
            targetProgramIndex = instr.cachedProgramIndex;
        } else {
            // Cache miss — full resolution with type-signature-based overload lookup

            // Build type signature from runtime argument types
            std::string typeSignature;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) typeSignature += ",";
                const auto& arg = args[i];
                if (std::holds_alternative<int64_t>(arg))
                {
                    typeSignature += "int";
                }
                else if (std::holds_alternative<double>(arg))
                {
                    typeSignature += "float";
                }
                else if (std::holds_alternative<bool>(arg))
                {
                    typeSignature += "bool";
                }
                else if (std::holds_alternative<std::string>(arg))
                {
                    typeSignature += "string";
                }
                else if (std::holds_alternative<value::InternedString>(arg))
                {
                    typeSignature += "string";
                }
                else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg))
                {
                    auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg);
                    typeSignature += obj->getTypeName();
                }
                else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arg))
                {
                    auto arr = std::get<std::shared_ptr<value::NativeArray>>(arg);
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
                else if (std::holds_alternative<std::shared_ptr<BytecodeLambda>>(arg))
                {
                    typeSignature += "function";
                }
                else if (std::holds_alternative<std::monostate>(arg) || std::holds_alternative<nullptr_t>(arg))
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

            // Populate cache for subsequent calls
            if (funcMetadata) {
                instr.cachedFuncMetadata = funcMetadata;
                instr.cachedStartOffset = funcMetadata->startOffset;
                instr.cachedProgram = targetProgram;
                instr.cachedProgramIndex = targetProgramIndex;
            }
        }

        if (funcMetadata)
        {
            // Convert lambda arguments to interface implementations if needed
            convertLambdaArgumentsToInterfaces(args, funcMetadata->parameterTypes);

            // Create call frame for static method
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase; // Use the frameBase calculated before popping args
            frame.localBase = context.stackManager->size(); // Locals start after arguments (which are now popped)
            frame.functionName = staticQualifiedName; // Use $static suffix for proper async method detection
            frame.thisInstance = nullptr; // No 'this' for static methods
            frame.programIndex = targetProgramIndex;

            // Switch to library program if function is from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(frame);
            context.stats.functionCalls++;

            vm::profiler::ProfilerHookHelper::onFunctionEntry(staticQualifiedName);

            // Notify debugger of static method entry
            if (debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc)
                {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(staticQualifiedName, errorsLoc);
                }
                else
                {
                    // Fallback: use function start location if current instruction has no location
                    auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (funcStartLoc)
                    {
                        errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                         funcStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(staticQualifiedName, errorsLoc);
                    }
                    else
                    {
                        debugger::DebugHookHelper::enterFunctionHook(staticQualifiedName, errors::SourceLocation());
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
        std::vector<value::Value>& args,
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
            if (paramType == "int" && std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg);
                if (obj->getTypeName() == "Int")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (std::holds_alternative<int64_t>(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "float" && std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg);
                if (obj->getTypeName() == "Float")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (std::holds_alternative<double>(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "bool" && std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg);
                if (obj->getTypeName() == "Bool")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (std::holds_alternative<bool>(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "string" && std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg))
            {
                auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(arg);
                if (obj->getTypeName() == "String")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (std::holds_alternative<std::string>(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }

            // AUTO-BOXING: Convert primitives to wrapper objects (Int, Float, Bool, String)
            if (paramType == "Int" && std::holds_alternative<int64_t>(arg))
            {
                // Auto-box int to Int
                auto intClass = context.environment->findClass("Int");
                if (intClass)
                {
                    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(intClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Float" && std::holds_alternative<double>(arg))
            {
                // Auto-box float to Float
                auto floatClass = context.environment->findClass("Float");
                if (floatClass)
                {
                    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(floatClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Bool" && std::holds_alternative<bool>(arg))
            {
                // Auto-box bool to Bool
                auto boolClass = context.environment->findClass("Bool");
                if (boolClass)
                {
                    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(boolClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "String" && std::holds_alternative<std::string>(arg))
            {
                // Auto-box string to String
                auto stringClass = context.environment->findClass("String");
                if (stringClass)
                {
                    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(stringClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }

            // BytecodeLambda (bytecode VM) - no conversion needed
            // ObjectExecutor::handleCallMethod handles BytecodeLambda invocation directly
            if (std::holds_alternative<std::shared_ptr<BytecodeLambda>>(arg))
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
            if (context.callStack.back().thisInstance)
            {
                // Instance method context
                currentClassName = context.callStack.back().thisInstance->getClassDefinition()->getName();
            }
            else
            {
                // Static method context - extract class name from function name (ClassName::methodName)
                const std::string& funcName = context.callStack.back().functionName;
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
