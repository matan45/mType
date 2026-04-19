#include "ObjectInstanceHelper.hpp"
#include "../../MethodSignature.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../gc/GC.hpp"
#include <algorithm>
#include  <iostream>
namespace vm::runtime
{
    // Helper function to parse type arguments with proper bracket depth tracking
    static std::vector<std::string> parseTypeArguments(const std::string& typeArgsStr)
    {
        std::vector<std::string> typeArgs;
        std::string current;
        int depth = 0;

        for (char c : typeArgsStr)
        {
            if (c == '<')
            {
                depth++;
                current += c;
            }
            else if (c == '>')
            {
                depth--;
                current += c;
            }
            else if (c == ',' && depth == 0)
            {
                // Split at comma only if we're at depth 0 (not inside nested generics)
                if (!current.empty())
                {
                    // Trim whitespace
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        typeArgs.push_back(current.substr(start, end - start + 1));
                    }
                }
                current.clear();
            }
            else
            {
                current += c;
            }
        }

        // Add the last type argument
        if (!current.empty())
        {
            // Trim whitespace
            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
            {
                typeArgs.push_back(current.substr(start, end - start + 1));
            }
        }

        return typeArgs;
    }

    ObjectInstanceHelper::ObjectInstanceHelper(ExecutionContext& ctx)
        : context(ctx)
    {
    }

    std::string ObjectInstanceHelper::parseGenericTypeArguments(const std::string& fullClassName,
                                                               std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');

        if (genericStart == std::string::npos) {
            return baseClassName;
        }

        baseClassName = fullClassName.substr(0, genericStart);
        size_t genericEnd = fullClassName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            return baseClassName;
        }

        std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // Parse type arguments with proper bracket depth tracking
        // Correctly handles nested generics like "Container<HashMap<String, List<Int>>>"
        std::vector<std::string> typeArgs = parseTypeArguments(typeArgsStr);

        // Validate type arguments
        auto& typeRegistry = types::getGlobalTypeRegistry();
        for (const auto& typeArg : typeArgs) {
            if (typeArg.empty()) {
                throw errors::TypeException("Invalid empty type argument for generic class '" + baseClassName + "'");
            }

            if (typeArg == "void" && baseClassName == "Promise") {
                continue;
            }

            // PURE OOP: Primitives are now allowed as generic type arguments!
            // They are treated as their Box class equivalents (Int, Float, Bool, String)
            // No need to reject primitive types - they're now objects
        }

        // Map generic parameters to concrete types
        // Get the class definition to access generic parameters
        if (!context.environment) {
            throw errors::RuntimeException("Environment not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classRegistry = context.environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class definition not found when parsing generic type arguments for class: " + baseClassName);
        }

        const auto& genericParams = classDef->getGenericParameters();

        // Map each generic parameter to its corresponding type argument
        for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i) {
            genericTypeBindings[genericParams[i].name] = typeArgs[i];
        }

        return baseClassName;
    }

    std::vector<value::Value> ObjectInstanceHelper::prepareConstructorArguments(size_t argCount)
    {
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());
        return args;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstanceHelper::createObjectInstance(
        const std::string& baseClassName,
        const std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        auto classRegistry = context.environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available");
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + baseClassName);
        }

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef, genericTypeBindings);
        initializeObjectFields(instance, classDef);

        // GC: Register the newly created object with the garbage collector
        instance->registerWithGC();

        return instance;
    }

    void ObjectInstanceHelper::initializeObjectFields(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        std::vector<std::shared_ptr<runtimeTypes::klass::ClassDefinition>> hierarchy;
        auto current = classDef;
        int depth = 0;
        while (current) {
            if (depth++ > 100) {
                throw errors::RuntimeException("Circular inheritance detected in class hierarchy");
            }
            hierarchy.push_back(current);  // O(1) instead of O(n)
            current = current->getParentClass();
        }
        // Reverse once at end: O(n) total instead of O(n²)
        std::reverse(hierarchy.begin(), hierarchy.end());

        for (const auto& classInHierarchy : hierarchy) {
            for (const auto& [fieldName, fieldDef] : classInHierarchy->getInstanceFields()) {
                // Final fields are always written by the constructor: inline
                // initializers fire in the prologue, ctor-initialized fields
                // fire in the body. Pre-populating them here would make the
                // instance-final check in ObjectExecutor::SET_FIELD see the
                // field as already set and reject the legitimate first write.
                // FieldInitializationValidator catches uninitialized finals
                // at compile time.
                if (fieldDef->isFinal()) continue;

                value::Value initialValue = fieldDef->getValue();
                if (value::isVoid(initialValue)) {
                    switch (fieldDef->getType()) {
                        case value::ValueType::INT:
                            initialValue = static_cast<int64_t>(0);
                            break;
                        case value::ValueType::FLOAT:
                            initialValue = 0.0;
                            break;
                        case value::ValueType::BOOL:
                            initialValue = false;
                            break;
                        case value::ValueType::STRING:
                            initialValue = std::string("");
                            break;
                        default:
                            // Objects, arrays, lambdas initialize to null
                            initialValue = nullptr;
                            break;
                    }
                }
                instance->setField(fieldName, initialValue);
            }
        }
    }

    void ObjectInstanceHelper::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        // Operand[0] is classNameIndex, Operand[1] is argCount
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR can only be called from within an instance context");
        }

        auto instance = context.callStack.back().thisInstance;

        // IMPORTANT: Use the current class name from the operand, NOT the instance's class!
        // The instance might be a subclass (e.g., Dog), but we're in the Mammal constructor
        // and need to call Mammal's parent (Animal), not Dog's parent (Mammal).
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        std::string parentClassName = classDef->getParentClassName();

        // Extract base parent class name (strip generic type arguments)
        // e.g., "BaseContainer<T>" -> "BaseContainer"
        std::string baseParentClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseParentClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + baseParentClassName);
        }

        // Use type-aware constructor lookup for overload resolution
        auto parentConstructor = parentClass->findConstructorByTypes(args);
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + baseParentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build constructor name with type signature for overload resolution
        std::string typeSignature = parentConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? baseParentClassName + "::<init>"
            : baseParentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = constructorName;  // Use qualified name for proper exception handling
            frame.thisInstance = instance;
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for constructor
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(frame);
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of parent constructor entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    // Fallback: use constructor start location
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
        // Operand[0] is classNameIndex, Operand[1] is argCount
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR can only be called from within an instance context");
        }

        auto instance = context.callStack.back().thisInstance;

        // Find the current class
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        // Use type-aware constructor lookup for overload resolution
        auto targetConstructor = classDef->findConstructorByTypes(args);
        if (!targetConstructor) {
            throw errors::RuntimeException("No constructor found in class " + currentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build constructor name with type signature for overload resolution
        std::string typeSignature = targetConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? currentClassName + "::<init>"
            : currentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = constructorName;  // Use qualified name for proper exception handling
            frame.thisInstance = instance;
            frame.definingClassName = currentClassName;  // Same class as defining class
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(frame);
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of constructor delegation entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    // Fallback: use constructor start location
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

    void ObjectInstanceHelper::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, methodNameIndex, argCount)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = method name
        // operand[2] = argument count
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[1]);
        size_t argCount = instr.operands[2];

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            throw errors::RuntimeException("SUPER_INVOKE can only be called from within an instance context");
        }

        auto instance = context.callStack.back().thisInstance;

        // IMPORTANT: Use currentClassName from operand, NOT instance->getClassDefinition()
        // This prevents infinite recursion in multi-level inheritance
        // (e.g., AdvancedService -> DerivedService -> BaseService)
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        std::string parentClassName = classDef->getParentClassName();

        // Extract base parent class name (strip generic type arguments)
        // e.g., "BaseContainer<T>" -> "BaseContainer"
        std::string baseParentClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseParentClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + baseParentClassName);
        }

        auto method = parentClass->findMethod(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Method not found in parent class " + baseParentClassName + ": " + methodName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build the mangled name with signature for overloaded methods
        // Use MethodSignature to eliminate manual signature building and 'this' confusion
        auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
        std::string qualifiedName = signature.toMangledName(baseParentClassName, false);  // false = not static

        auto funcMetadata = context.program->getFunction(qualifiedName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(qualifiedName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = qualifiedName;
            frame.thisInstance = instance;
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for access control
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(frame);
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of super method entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                } else {
                    // Fallback: use method start location if current instruction has no location
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
        // Compiler emits: (classNameIndex, memberNameIndex)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.operands[1]);

        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            throw errors::RuntimeException("SUPER_GET_FIELD can only be called from within an instance context");
        }

        auto instance = context.callStack.back().thisInstance;

        // Get the current class definition
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        // Get field value from the instance (fields are stored on the instance, not the class)
        value::Value fieldValue = instance->getFieldValue(memberName);

        // Check if field exists
        if (value::isVoid(fieldValue)) {
            throw errors::RuntimeException("Field '" + memberName + "' not found in parent class");
        }

        // Push the field value onto the stack
        context.stackManager->push(fieldValue);
    }

    void ObjectInstanceHelper::handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, memberNameIndex)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.operands[1]);

        if (context.callStack.empty() || !context.callStack.back().thisInstance) {
            throw errors::RuntimeException("SUPER_SET_FIELD can only be called from within an instance context");
        }

        auto instance = context.callStack.back().thisInstance;

        // Get the current class definition
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        // Pop the value to assign from the stack
        value::Value assignValue = context.stackManager->pop();

        // Set field value on the instance (fields are stored on the instance, not the class)
        instance->setField(memberName, assignValue);
    }

    void ObjectInstanceHelper::invokeConstructor(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                                const std::string& baseClassName,
                                                const std::vector<value::Value>& args)
    {
        size_t argCount = args.size();
        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseClassName);

        // Use type-aware constructor lookup for overload resolution
        auto constructor = classDef->findConstructorByTypes(args);
        if (!constructor) {
            bool hasAnyConstructor = !classDef->getConstructors().empty();

            if (argCount == 0 && !hasAnyConstructor) {
                context.stackManager->push(instance);
                return;
            }

            throw errors::RuntimeException("No constructor found for " + baseClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        auto accessContext = createAccessContext(baseClassName, false);
        validation::AccessValidator::validateConstructorAccess(baseClassName, constructor->getAccessModifier(), accessContext);

        // Build type signature from runtime argument values for overload resolution
        std::string typeSignature = constructor->getTypeSignature();

        std::string constructorName = typeSignature.empty()
            ? baseClassName + "::<init>"
            : baseClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (!funcMetadata) {
            throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName +
                                         "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        frame.functionName = constructorName;  // Use qualified name for proper exception handling
        frame.thisInstance = instance;
        frame.definingClassName = baseClassName;  // Set class as defining class for its own constructor
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(frame);
        context.stats.functionCalls++;

        // Switch to the target program if it's from a library
        if (targetProgram != context.program) {
            context.program = targetProgram;
        }

        vm::profiler::ProfilerHookHelper::onFunctionEntry(constructorName);

        // Notify debugger of constructor entry
        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
            } else {
                // Fallback: use constructor start location if current instruction has no location
                auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                if (ctorStartLoc) {
                    errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                }
            }
        }

        context.stackManager->push(instance);
        for (size_t i = 0; i < argCount; ++i) {
            context.stackManager->push(args[i]);
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void ObjectInstanceHelper::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("NEW_OBJECT requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];


        // Parse generic type arguments and extract base class name
        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeArguments(fullClassName, genericTypeBindings);

        // Prepare constructor arguments from stack
        std::vector<value::Value> args = prepareConstructorArguments(argCount);

        // PHASE 2 OPTIMIZATION: Integer Caching
        // If creating Int object with single int argument in cacheable range, use cached instance
        if (baseClassName == "Int" && argCount == 1 && value::isInt(args[0])) {
            int intValue = static_cast<int>(value::asInt(args[0]));

            // Check if value is cacheable
            if (value::IntegerCache::isCacheable(intValue)) {
                // Get Int class definition for cache
                auto classRegistry = context.environment->getClassRegistry();
                auto intClassDef = classRegistry ? classRegistry->findClass("Int") : nullptr;

                if (intClassDef) {
                    // Try to get from cache
                    auto cachedInstance = value::IntegerCache::getInt(intValue, intClassDef);

                    if (cachedInstance) {
                        // Cache hit! Return cached Int object (already initialized)
                        // Skip constructor invocation - cached object is already properly initialized
                        invokeConstructor(cachedInstance, baseClassName, args);
                        return;
                    }
                }
            }
        }

        // Normal object creation path (non-cached or cache miss)
        auto instance = createObjectInstance(baseClassName, genericTypeBindings);

        // Invoke constructor using the class definition's actual name
        // (handles aliases: "MyInt" resolves to same ClassDef as "Int",
        //  but constructor bytecode is registered under "Int::<init>")
        std::string actualClassName = instance->getClassDefinition()->getName();
        invokeConstructor(instance, actualClassName, args);
    }

    std::string ObjectInstanceHelper::getCurrentClassName() {
        if (!context.callStack.empty()) {
            if (context.callStack.back().thisInstance) {
                return context.callStack.back().thisInstance->getClassDefinition()->getName();
            } else {
                const std::string& funcName = context.callStack.back().functionName;
                size_t colonPos = funcName.find("::");
                if (colonPos != std::string::npos) {
                    std::string className = funcName.substr(0, colonPos);
                    return className;
                }
            }
        }
        return "";
    }

    bool ObjectInstanceHelper::isSubclass(const std::string& derivedClass, const std::string& baseClass) {
        if (derivedClass.empty()) return false;
        auto currentClass = context.environment->getClassRegistry()->findClass(derivedClass);
        while (currentClass && currentClass->hasParentClass()) {
            std::string parentClassName = currentClass->getParentClassName();

            // Extract base parent class name (strip generic type arguments)
            std::string baseParentClassName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentClassName = parentClassName.substr(0, genericStart);
            }

            if (baseParentClassName == baseClass) {
                return true;
            }
            auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
            currentClass = parentClass;
        }
        return false;
    }

    validation::AccessContext ObjectInstanceHelper::createAccessContext(
        const std::string& targetClassName,
        bool isSetter)
    {
        std::string currentClassName = getCurrentClassName();
        bool isSameClass = (currentClassName == targetClassName);
        bool isSubclassCheck = isSubclass(currentClassName, targetClassName);

        // Special case: Static field initialization (SET operations) happens in global scope
        // Allow SET during static initialization
        if (currentClassName.empty() && context.callStack.empty() && isSetter) {
            // Allow initialization by treating it as same class access
            isSameClass = true;
        }

        return validation::AccessContext(
            currentClassName,
            targetClassName,
            isSameClass,
            isSubclassCheck,
            isSetter,
            errors::SourceLocation()
        );
    }
}
