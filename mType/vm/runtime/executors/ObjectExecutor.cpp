#include "ObjectExecutor.hpp"
#include "FunctionExecutor.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../value/LambdaValue.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include <algorithm>

namespace vm::runtime
{
    ObjectExecutor::ObjectExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void ObjectExecutor::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("NEW_OBJECT requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];


        std::string baseClassName = fullClassName;
        std::unordered_map<std::string, std::string> genericTypeBindings;

        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genericStart);
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd != std::string::npos && genericEnd > genericStart) {
                std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

                // Validate generic type arguments
                // Parse type arguments (simple comma-separated for now)
                std::vector<std::string> typeArgs;
                size_t start = 0;
                size_t commaPos;
                while ((commaPos = typeArgsStr.find(',', start)) != std::string::npos) {
                    std::string typeArg = typeArgsStr.substr(start, commaPos - start);
                    // Trim whitespace
                    typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                    typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                    typeArgs.push_back(typeArg);
                    start = commaPos + 1;
                }
                // Add last type argument
                if (start < typeArgsStr.length()) {
                    std::string typeArg = typeArgsStr.substr(start);
                    typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                    typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                    typeArgs.push_back(typeArg);
                }

                // Validate that type arguments are not primitive types
                // EXCEPTION: Promise<void> is allowed for async functions
                auto& typeRegistry = types::getGlobalTypeRegistry();
                for (const auto& typeArg : typeArgs) {
                    if (typeArg.empty()) {
                        throw errors::TypeException("Invalid empty type argument for generic class '" + baseClassName + "'");
                    }

                    // Allow void only for Promise type (used in async functions)
                    if (typeArg == "void" && baseClassName == "Promise") {
                        continue;
                    }

                    // Reject primitive types
                    if (typeRegistry.isPrimitiveType(typeArg)) {
                        throw errors::TypeException(
                            "Generic type arguments must be object types (classes/interfaces) or generic parameters. "
                            "Primitive type '" + typeArg + "' is not allowed as a generic argument for class '" + baseClassName + "'.");
                    }
                }
            }
        }

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

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

        auto constructor = classDef->findConstructor(argCount);
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

        std::string constructorName = baseClassName + "::<init>/" + std::to_string(argCount);
        auto funcMetadata = context.program->getFunction(constructorName);
        if (funcMetadata) {
            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = context.stackManager->size();
            frame.localBase = context.stackManager->size();  // Set before pushing
            frame.functionName = "<init>";
            frame.thisInstance = instance;

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            // Push this and arguments AFTER setting up the frame
            // They will be at positions localBase+0, localBase+1, etc.
            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            // Note: Local variables are allocated on-demand by STORE_LOCAL
            // which extends the stack as needed (see VariableExecutor::handleStoreLocal)

            context.instructionPointer = funcMetadata->startOffset - 1;
            return;
        }

        throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName + "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
    }

    void ObjectExecutor::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("GET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object", errorLoc);
            } else {
                throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object");
            }
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("GET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        // Validate access control
        std::string targetClassName = instance->getClassDefinition()->getName();
        auto accessContext = createAccessContext(targetClassName, false);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        value::Value fieldValue = instance->getFieldValue(fieldName);
        context.stackManager->push(fieldValue);
    }

    void ObjectExecutor::handleSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("SET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object", errorLoc);
            } else {
                throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object");
            }
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("SET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        if (fieldDef->isFinal()) {
            // Allow initialization of final fields (when not yet initialized)
            if (fieldDef->isInitialized()) {
                auto* loc = context.program->getSourceLocation(context.instructionPointer);
                if (loc) {
                    errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                    throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'", errorLoc);
                } else {
                    throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
                }
            }
        }

        // Validate access control
        std::string targetClassName = instance->getClassDefinition()->getName();
        auto accessContext = createAccessContext(targetClassName, true);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        instance->setField(fieldName, newValue);

        // Push the value back onto the stack for chained assignments
        // This allows expressions like: obj1.field = obj2.field = value
        context.stackManager->push(newValue);
    }

    void ObjectExecutor::handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("GET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            throw errors::RuntimeException("GET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (!fieldDef->isStatic()) {
            throw errors::RuntimeException("Field '" + fieldName + "' is not static");
        }

        auto accessContext = createAccessContext(className, false);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        value::Value fieldValue = fieldDef->getValue();
        context.stackManager->push(fieldValue);
    }

    void ObjectExecutor::handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("SET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            throw errors::RuntimeException("SET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (!fieldDef->isStatic()) {
            throw errors::RuntimeException("Field '" + fieldName + "' is not static");
        }

        if (fieldDef->isFinal()) {
            // Allow initialization of final fields (when not yet initialized)
            if (fieldDef->isInitialized()) {
                auto* loc = context.program->getSourceLocation(context.instructionPointer);
                if (loc) {
                    errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                    throw errors::RuntimeException("Cannot assign to final static field '" + qualifiedName + "'", errorLoc);
                } else {
                    throw errors::RuntimeException("Cannot assign to final static field '" + qualifiedName + "'");
                }
            }
        }

        auto accessContext = createAccessContext(className, true);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        fieldDef->setValue(newValue);
    }

    void ObjectExecutor::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());

        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object", errorLoc);
            } else {
                throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object");
            }
        }

        // Check if this is a lambda (interface method invocation)
        if (std::holds_alternative<std::shared_ptr<BytecodeLambda>>(objectValue)) {
            auto lambda = std::get<std::shared_ptr<BytecodeLambda>>(objectValue);

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
            frame.originatingLambda = lambda;  // Store lambda reference for variable access

            context.callStack.push_back(frame);

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
            return;
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("CALL_METHOD requires an object instance or lambda");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        auto classDef = instance->getClassDefinition();

        // Use findInstanceMethodInHierarchy to search only instance methods in parent classes
        auto method = classDef->findInstanceMethodInHierarchy(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Instance method not found: " + methodName +
                                         " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        auto accessContext = createAccessContext(classDef->getName(), false);
        validation::AccessValidator::validateMethodAccess(methodName, method->getAccessModifier(), accessContext);

        // Find which class actually defines this method by walking up the hierarchy
        std::string definingClassName = classDef->getName();
        auto currentClass = classDef;
        while (currentClass) {
            auto localMethod = currentClass->findInstanceMethod(methodName, argCount);
            if (localMethod) {
                definingClassName = currentClass->getName();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        std::string qualifiedName = definingClassName + "::" + methodName;
        auto funcMetadata = context.program->getFunction(qualifiedName);
        if (funcMetadata) {
            // Convert lambda arguments to interface implementations if needed
            if (functionExecutor) {
                functionExecutor->convertLambdaArgumentsToInterfaces(args, funcMetadata->parameterTypes);
            }

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

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }
    }

    void ObjectExecutor::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
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
        auto parentClass = context.environment->getClassRegistry()->findClass(parentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + parentClassName);
        }

        auto parentConstructor = parentClass->findConstructor(argCount);
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + parentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        std::string constructorName = parentClassName + "::<init>/" + std::to_string(argCount);
        auto funcMetadata = context.program->getFunction(constructorName);
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
            frame.functionName = "<init>";
            frame.thisInstance = instance;

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent constructor '" + constructorName + "' has no bytecode.");
        }
    }

    void ObjectExecutor::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
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
        auto parentClass = context.environment->getClassRegistry()->findClass(parentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + parentClassName);
        }

        auto method = parentClass->findMethod(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Method not found in parent class " + parentClassName + ": " + methodName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        std::string qualifiedName = parentClassName + "::" + methodName;
        auto funcMetadata = context.program->getFunction(qualifiedName);
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

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent method '" + qualifiedName + "' has no bytecode.");
        }
    }

    void ObjectExecutor::initializeObjectFields(
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
            hierarchy.insert(hierarchy.begin(), current);
            current = current->getParentClass();
        }

        for (const auto& classInHierarchy : hierarchy) {
            for (const auto& [fieldName, fieldDef] : classInHierarchy->getInstanceFields()) {
                value::Value initialValue = fieldDef->getValue();
                if (std::holds_alternative<std::monostate>(initialValue)) {
                    switch (fieldDef->getType()) {
                        case value::ValueType::INT:
                            initialValue = 0;
                            break;
                        case value::ValueType::FLOAT:
                            initialValue = 0.0f;
                            break;
                        case value::ValueType::BOOL:
                            initialValue = false;
                            break;
                        case value::ValueType::STRING:
                            initialValue = std::string("");
                            break;
                        default:
                            initialValue = std::monostate{};
                            break;
                    }
                }
                instance->setField(fieldName, initialValue);
            }
        }
    }

    std::string ObjectExecutor::getCurrentClassName() {
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

    bool ObjectExecutor::isSubclass(const std::string& derivedClass, const std::string& baseClass) {
        if (derivedClass.empty()) return false;
        auto currentClass = context.environment->getClassRegistry()->findClass(derivedClass);
        while (currentClass && currentClass->hasParentClass()) {
            if (currentClass->getParentClassName() == baseClass) {
                return true;
            }
            auto parentClass = context.environment->getClassRegistry()->findClass(currentClass->getParentClassName());
            currentClass = parentClass;
        }
        return false;
    }

    validation::AccessContext ObjectExecutor::createAccessContext(
        const std::string& targetClassName,
        bool isSetter
    )
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
