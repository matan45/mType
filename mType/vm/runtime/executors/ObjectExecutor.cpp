#include "ObjectExecutor.hpp"
#include "ObjectInstanceHelper.hpp"
#include "FunctionExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include <algorithm>
#include <iostream>
namespace vm::runtime
{
    ObjectExecutor::ObjectExecutor(ExecutionContext& ctx)
        : context(ctx)
        , instanceHelper(std::make_unique<ObjectInstanceHelper>(ctx))
    {}

    ObjectExecutor::~ObjectExecutor() = default;

    void ObjectExecutor::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleNewObject(instr);
    }

    void ObjectExecutor::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot access field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        // Validate access control
        // IMPORTANT: Use the class that OWNS the field, not the runtime class of the instance
        // This is critical for private field access validation in inheritance
        auto classDef = instance->getClassDefinition();
        auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
        std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();

        auto accessContext = createAccessContext(targetClassName, false);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        value::Value fieldValue = instance->getFieldValue(fieldName);
        context.stackManager->push(fieldValue);
    }

    void ObjectExecutor::handleSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot set field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires an object instance");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        if (fieldDef->isFinal()) {
            // Allow initialization of final fields (when not yet initialized)
            if (fieldDef->isInitialized()) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Cannot assign to final field '" + fieldName + "'");
            }
        }

        // Validate access control
        // IMPORTANT: Use the class that OWNS the field, not the runtime class of the instance
        // This is critical for private field access validation in inheritance
        auto classDef = instance->getClassDefinition();
        auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
        std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();

        auto accessContext = createAccessContext(targetClassName, true);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        instance->setField(fieldName, newValue);

        // Push the value back onto the stack for chained assignments
        // This allows expressions like: obj1.field = obj2.field = value
        context.stackManager->push(newValue);
    }

    void ObjectExecutor::handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "GET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Class not found: " + className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (!fieldDef->isStatic()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Field '" + fieldName + "' is not static");
        }

        auto accessContext = createAccessContext(className, false);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        value::Value fieldValue = fieldDef->getValue();
        context.stackManager->push(fieldValue);
    }

    void ObjectExecutor::handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "SET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Class not found: " + className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (!fieldDef->isStatic()) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Field '" + fieldName + "' is not static");
        }

        if (fieldDef->isFinal()) {
            // Allow initialization of final fields (when not yet initialized)
            if (fieldDef->isInitialized()) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Cannot assign to final static field '" + qualifiedName + "'");
            }
        }

        auto accessContext = createAccessContext(className, true);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        fieldDef->setValue(newValue);
    }

    std::vector<value::Value> ObjectExecutor::prepareMethodCallArguments(size_t argCount) {
        std::vector<value::Value> args;
        args.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) {
            args.push_back(context.stackManager->pop());
        }
        std::reverse(args.begin(), args.end());
        return args;
    }

    void ObjectExecutor::invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                                           const std::vector<value::Value>& args,
                                           const std::string& methodName) {
        size_t lambdaStart = lambda->instructionPointer;
        size_t paramCount = lambda->parameterCount;

        // Validate argument count
        if (args.size() != paramCount) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Lambda expects " + std::to_string(paramCount) +
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
        frame.definingClassName = lambda->creatingClassName;  // Set creating class for access control

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

        // Reserve additional local variable slots if needed (for local variables like return value temporaries)
        // Look up lambda metadata to get localCount
        auto* lambdaMetadata = context.program->getFunction(lambda->functionName);
        if (lambdaMetadata) {
            size_t pushedSlots = args.size() + lambda->capturedValues.size();  // parameters + captured
            if (lambdaMetadata->localCount > pushedSlots) {
                size_t additionalLocals = lambdaMetadata->localCount - pushedSlots;
                for (size_t i = 0; i < additionalLocals; ++i) {
                    context.stackManager->push(std::monostate{});
                }
            }
        }

        // Jump to lambda start (subtract 1 because the VM loop will increment after this)
        context.instructionPointer = lambdaStart - 1;
    }

    void ObjectExecutor::invokeInstanceMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                             const std::string& methodName,
                                             const std::vector<value::Value>& args,
                                             size_t argCount) {
        auto classDef = instance->getClassDefinition();

        // Use findInstanceMethodInHierarchy to search only instance methods in parent classes
        auto method = classDef->findInstanceMethodInHierarchy(methodName, argCount);
        if (!method) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Instance method not found: " + methodName +
                " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        // Find which class actually defines this method by walking up the hierarchy
        // IMPORTANT: Do this BEFORE access validation so we use the correct defining class
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

        // Validate method access using the defining class, not the runtime instance class
        auto accessContext = createAccessContext(definingClassName, false);
        validation::AccessValidator::validateMethodAccess(methodName, method->getAccessModifier(), accessContext);

        std::string qualifiedName = definingClassName + "::" + methodName;
        auto funcMetadata = context.program->getFunction(qualifiedName);
        if (!funcMetadata) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }

        // Convert lambda arguments to interface implementations if needed
        std::vector<value::Value> modifiedArgs = args;
        if (functionExecutor) {
            functionExecutor->convertLambdaArgumentsToInterfaces(modifiedArgs, funcMetadata->parameterTypes);
        }

        size_t frameBase = context.stackManager->size();
        context.stackManager->push(instance);

        for (size_t i = 0; i < argCount; ++i) {
            context.stackManager->push(modifiedArgs[i]);
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = frameBase;
        frame.functionName = qualifiedName;
        frame.thisInstance = instance;
        frame.definingClassName = definingClassName;  // Store the class that defines this method

        context.pushCallFrame(frame);
        context.stats.functionCalls++;

        // Notify debugger of method entry
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

        // Reserve and initialize local variable slots (beyond 'this' and parameters)
        // localCount for instance methods includes 'this' (slot 0) + parameters + locals
        // We've already pushed 'this' and arguments, so reserve (localCount - 1 - argCount) additional slots
        size_t pushedSlots = 1 + argCount;  // 'this' + arguments
        if (funcMetadata->localCount > pushedSlots) {
            size_t additionalLocals = funcMetadata->localCount - pushedSlots;
            for (size_t i = 0; i < additionalLocals; ++i) {
                context.stackManager->push(std::monostate{});
            }
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void ObjectExecutor::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Prepare arguments from stack
        std::vector<value::Value> args = prepareMethodCallArguments(argCount);

        // Pop object and check for null
        value::Value objectValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(objectValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot call method '" + methodName + "' on null object");
        }

        // Handle lambda invocation
        if (std::holds_alternative<std::shared_ptr<BytecodeLambda>>(objectValue)) {
            auto lambda = std::get<std::shared_ptr<BytecodeLambda>>(objectValue);
            invokeLambdaMethod(lambda, args, methodName);
            return;
        }

        // Handle regular instance method invocation
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "CALL_METHOD requires an object instance or lambda");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        invokeInstanceMethod(instance, methodName, args, argCount);
    }

    void ObjectExecutor::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleSuperConstructor(instr);
    }

    void ObjectExecutor::handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleThisConstructor(instr);
    }

    void ObjectExecutor::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleSuperInvoke(instr);
    }

    void ObjectExecutor::handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleSuperGetField(instr);
    }

    void ObjectExecutor::handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleSuperSetField(instr);
    }

    std::string ObjectExecutor::getCurrentClassName() {
        if (!context.callStack.empty()) {
            // IMPORTANT: Use the defining class from the CallFrame, not the runtime class
            // This is critical for private field access validation in inheritance
            if (!context.callStack.back().definingClassName.empty()) {
                return context.callStack.back().definingClassName;
            }
            // Fallback to instance class if defining class not set (e.g., for constructors)
            if (context.callStack.back().thisInstance) {
                return context.callStack.back().thisInstance->getClassDefinition()->getName();
            } else {
                // Fallback: extract class name from function name (static methods)
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

    validation::AccessContext ObjectExecutor::createAccessContext(
        const std::string& targetClassName,
        bool isSetter
    )
    {
        std::string currentClassName = getCurrentClassName();
        bool isSameClass = (currentClassName == targetClassName);
        bool isSubclassCheck = isSubclass(currentClassName, targetClassName);

        // Special case: Static field initialization (SET operations) happens in global scope
        // Allow SET during static initialization (either no call stack or in __script_main__)
        bool inScriptMain = !context.callStack.empty() &&
                           context.callStack.back().functionName == "__script_main__";
        if (currentClassName.empty() && (context.callStack.empty() || inScriptMain) && isSetter) {
            // Allow initialization by treating it as same class access
            isSameClass = true;
        }

        // Get the current source location from execution context
        errors::SourceLocation location(
            context.currentSourceFile,
            context.currentSourceLine,
            1  // Column information not currently tracked
        );

        return validation::AccessContext(
            currentClassName,
            targetClassName,
            isSameClass,
            isSubclassCheck,
            isSetter,
            location
        );
    }
}
