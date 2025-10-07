#include "ObjectExecutor.hpp"
#include "../../../errors/SourceLocation.hpp"
#include <algorithm>
#include <iostream>

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

        validateConstructorAccess(baseClassName, constructor->getAccessModifier());

        std::string constructorName = baseClassName + "::<init>/" + std::to_string(argCount);
        auto funcMetadata = context.program->getFunction(constructorName);
        if (funcMetadata) {
            size_t localBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            // For constructors, frameBase = localBase (typically 0)
            // When the constructor returns via RETURN_VALUE:
            // 1. It pops the return value (the instance)
            // 2. handleReturn() clears the stack to frameBase
            // 3. It pushes the return value (instance) back
            // This leaves only the instance on the stack
            frame.frameBase = localBase;
            frame.localBase = localBase;
            frame.functionName = "<init>";
            frame.thisInstance = instance;

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName + "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }
    }

    void ObjectExecutor::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("GET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("GET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        ast::AccessModifier accessMod = fieldDef->getAccessModifier();
        if (accessMod != ast::AccessModifier::PUBLIC) {
            std::string currentClassName;
            if (!context.callStack.empty() && context.callStack.back().thisInstance) {
                currentClassName = context.callStack.back().thisInstance->getClassDefinition()->getName();
            }

            std::string targetClassName = instance->getClassDefinition()->getName();

            if (accessMod == ast::AccessModifier::PRIVATE) {
                if (currentClassName != targetClassName) {
                    std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                    throw errors::AccessViolationException(
                        fieldName, "field", ast::AccessModifier::PRIVATE,
                        targetClassName, callingFrom, errors::SourceLocation()
                    );
                }
            } else if (accessMod == ast::AccessModifier::PROTECTED) {
                if (currentClassName != targetClassName) {
                    bool isSubclass = false;
                    if (!currentClassName.empty()) {
                        auto currentClass = context.environment->getClassRegistry()->findClass(currentClassName);
                        while (currentClass && currentClass->hasParentClass()) {
                            if (currentClass->getParentClassName() == targetClassName) {
                                isSubclass = true;
                                break;
                            }
                            auto parentClass = context.environment->getClassRegistry()->findClass(currentClass->getParentClassName());
                            currentClass = parentClass;
                        }
                    }

                    if (!isSubclass) {
                        std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                        throw errors::AccessViolationException(
                            fieldName, "field", ast::AccessModifier::PROTECTED,
                            targetClassName, callingFrom, errors::SourceLocation()
                        );
                    }
                }
            }
        }

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
            throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object");
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
            throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
        }

        ast::AccessModifier accessMod = fieldDef->getAccessModifier();
        if (accessMod != ast::AccessModifier::PUBLIC) {
            std::string currentClassName;
            if (!context.callStack.empty() && context.callStack.back().thisInstance) {
                currentClassName = context.callStack.back().thisInstance->getClassDefinition()->getName();
            }

            std::string targetClassName = instance->getClassDefinition()->getName();

            if (accessMod == ast::AccessModifier::PRIVATE) {
                if (currentClassName != targetClassName) {
                    std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                    throw errors::AccessViolationException(
                        fieldName, "field", ast::AccessModifier::PRIVATE,
                        targetClassName, callingFrom, errors::SourceLocation()
                    );
                }
            } else if (accessMod == ast::AccessModifier::PROTECTED) {
                if (currentClassName != targetClassName) {
                    bool isSubclass = false;
                    if (!currentClassName.empty()) {
                        auto currentClass = context.environment->getClassRegistry()->findClass(currentClassName);
                        while (currentClass && currentClass->hasParentClass()) {
                            if (currentClass->getParentClassName() == targetClassName) {
                                isSubclass = true;
                                break;
                            }
                            auto parentClass = context.environment->getClassRegistry()->findClass(currentClass->getParentClassName());
                            currentClass = parentClass;
                        }
                    }

                    if (!isSubclass) {
                        std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                        throw errors::AccessViolationException(
                            fieldName, "field", ast::AccessModifier::PROTECTED,
                            targetClassName, callingFrom, errors::SourceLocation()
                        );
                    }
                }
            }
        }

        instance->setField(fieldName, newValue);
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

        validateFieldAccess(className, fieldName, fieldDef->getAccessModifier(), false);

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
            throw errors::RuntimeException("Cannot assign to final static field '" + qualifiedName + "'");
        }

        validateFieldAccess(className, fieldName, fieldDef->getAccessModifier(), true);

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
            throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw errors::RuntimeException("CALL_METHOD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        auto classDef = instance->getClassDefinition();

        // Use findMethodInHierarchy to search in parent classes too
        auto method = classDef->findMethodInHierarchy(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Method not found: " + methodName +
                                         " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        if (method->isStatic()) {
            throw errors::RuntimeException("Cannot call static method '" + methodName + "' on an instance. Use static method call instead.");
        }

        validateMethodAccess(classDef->getName(), methodName, method->getAccessModifier());

        // Find which class actually defines this method by walking up the hierarchy
        std::string definingClassName = classDef->getName();
        auto currentClass = classDef;
        while (currentClass) {
            auto localMethod = currentClass->findMethod(methodName, argCount);
            if (localMethod) {
                definingClassName = currentClass->getName();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        std::string qualifiedName = definingClassName + "::" + methodName;
        auto funcMetadata = context.program->getFunction(qualifiedName);
        if (funcMetadata) {
            size_t localBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = localBase;
            frame.localBase = localBase;
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
            // localBase is where locals start (after instance + args are pushed)
            size_t localBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            // For super constructor calls, use localBase for frameBase
            // When parent constructor returns, stack restores to this position
            frame.frameBase = localBase;
            frame.localBase = localBase;
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
        // We need operand[1] for method name and operand[2] for argCount
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
        auto classDef = instance->getClassDefinition();

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
            size_t localBase = context.stackManager->size();

            context.stackManager->push(instance);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = localBase;
            frame.localBase = localBase;
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
                    return funcName.substr(0, colonPos);
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

    void ObjectExecutor::validateConstructorAccess(const std::string& className, ast::AccessModifier accessMod) {
        if (accessMod == ast::AccessModifier::PUBLIC) return;

        std::string currentClassName = getCurrentClassName();

        if (accessMod == ast::AccessModifier::PRIVATE) {
            if (currentClassName != className) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    className + " constructor", "constructor",
                    ast::AccessModifier::PRIVATE, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        } else if (accessMod == ast::AccessModifier::PROTECTED) {
            if (currentClassName != className && !isSubclass(currentClassName, className)) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    className + " constructor", "constructor",
                    ast::AccessModifier::PROTECTED, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        }
    }

    void ObjectExecutor::validateMethodAccess(const std::string& className, const std::string& methodName, ast::AccessModifier accessMod) {
        if (accessMod == ast::AccessModifier::PUBLIC) return;

        std::string currentClassName = getCurrentClassName();

        if (accessMod == ast::AccessModifier::PRIVATE) {
            if (currentClassName != className) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    methodName, "method",
                    ast::AccessModifier::PRIVATE, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        } else if (accessMod == ast::AccessModifier::PROTECTED) {
            if (currentClassName != className && !isSubclass(currentClassName, className)) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    methodName, "method",
                    ast::AccessModifier::PROTECTED, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        }
    }

    void ObjectExecutor::validateFieldAccess(const std::string& className, const std::string& fieldName, ast::AccessModifier accessMod, bool isSetter) {
        if (accessMod == ast::AccessModifier::PUBLIC) return;

        std::string currentClassName = getCurrentClassName();

        // Special case: Static field initialization (SET operations) happens in global scope (empty call stack)
        // Allow SET during static initialization - the compiler ensures we only set fields
        // of the class being initialized
        // But REJECT GET operations from global scope (external access attempts)
        if (currentClassName.empty() && context.callStack.empty()) {
            if (isSetter) {
                return;  // Allow SET during initialization
            }
            // For GET, fall through to normal validation which will reject it
        }

        if (accessMod == ast::AccessModifier::PRIVATE) {
            if (currentClassName != className) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    fieldName, "field",
                    ast::AccessModifier::PRIVATE, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        } else if (accessMod == ast::AccessModifier::PROTECTED) {
            if (currentClassName != className && !isSubclass(currentClassName, className)) {
                std::string callingFrom = currentClassName.empty() ? "global scope" : currentClassName;
                throw errors::AccessViolationException(
                    fieldName, "field",
                    ast::AccessModifier::PROTECTED, className, callingFrom,
                    errors::SourceLocation()
                );
            }
        }
    }
}
