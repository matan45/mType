#include "ObjectInstanceHelper.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../evaluator/utils/GenericTypeManager.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include <algorithm>

namespace vm::runtime
{
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

        // Use GenericTypeManager for robust parsing with proper depth tracking
        // CRITICAL FIX: Previous implementation failed on nested generics like "Container<HashMap<String, List<Int>>>"
        // It would incorrectly split on inner commas, producing ["HashMap<String", "List<Int>>"]
        // GenericTypeManager correctly handles bracket depth to produce ["HashMap<String, List<Int>>"]
        std::vector<std::string> typeArgs = evaluator::utils::GenericTypeManager::parseTypeArguments(typeArgsStr);

        // Validate type arguments
        auto& typeRegistry = types::getGlobalTypeRegistry();
        for (const auto& typeArg : typeArgs) {
            if (typeArg.empty()) {
                throw errors::TypeException("Invalid empty type argument for generic class '" + baseClassName + "'");
            }

            if (typeArg == "void" && baseClassName == "Promise") {
                continue;
            }

            if (typeRegistry.isPrimitiveType(typeArg)) {
                throw errors::TypeException(
                    "Generic type arguments must be object types (classes/interfaces) or generic parameters. "
                    "Primitive type '" + typeArg + "' is not allowed as a generic argument for class '" + baseClassName + "'.");
            }
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

        auto parentConstructor = parentClass->findConstructor(argCount);
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + baseParentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        std::string constructorName = baseParentClassName + "::<init>/" + std::to_string(argCount);
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
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for constructor

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

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

        std::string qualifiedName = baseParentClassName + "::" + methodName;
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
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for access control

            context.callStack.push_back(frame);
            context.stats.functionCalls++;

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
        if (std::holds_alternative<std::monostate>(fieldValue)) {
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
        if (!funcMetadata) {
            throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName +
                                         "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        frame.functionName = "<init>";
        frame.thisInstance = instance;
        frame.definingClassName = baseClassName;  // Set class as defining class for its own constructor

        context.callStack.push_back(frame);
        context.stats.functionCalls++;

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

        // Create object instance and initialize fields
        auto instance = createObjectInstance(baseClassName, genericTypeBindings);

        // Invoke constructor
        invokeConstructor(instance, baseClassName, args);
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
