#include "ObjectExecutor.hpp"
#include "ObjectInstanceHelper.hpp"
#include "FunctionExecutor.hpp"
#include "../../MethodSignature.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include <algorithm>
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
        // Use the lambda's unique function name (e.g., __lambda_0) for metadata/exception table lookup
        frame.functionName = lambda->functionName.empty() ?
            (lambda->creatingClassName.empty() ? "<lambda>" : lambda->creatingClassName + "::<lambda>") :
            lambda->functionName;
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

        // Create a SharedStackFrame for this lambda invocation to support nested closures
        // Link it to the parent frame so nested lambdas can access parent variables
        auto newSharedFrame = std::make_shared<SharedStackFrame>();
        newSharedFrame->parentFrame = lambda->capturedFrame;  // Link to parent
        if (!context.callStack.empty()) {
            context.callStack.back().sharedFrame = newSharedFrame;
        }

        // Get lambda metadata for parameter type information
        auto* lambdaMetadata = context.program->getFunction(lambda->functionName);

        // Push arguments onto stack (they become local variables at indices 0, 1, 2, ...)
        // Also register them by name in SharedStackFrame so nested lambdas can capture them
        for (size_t i = 0; i < args.size(); ++i) {
            value::Value argValue = args[i];

            // Auto-box primitive arguments if lambda expects boxed types
            if (lambdaMetadata && i < lambdaMetadata->parameterTypes.size()) {
                std::string expectedType = lambdaMetadata->parameterTypes[i];

                // Check if we need to box a primitive to a wrapper class
                bool needsBoxing = false;
                std::string boxClassName;

                if (expectedType == "Int" && std::holds_alternative<int>(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Int";
                }
                else if (expectedType == "Float" && (std::holds_alternative<float>(argValue) || std::holds_alternative<int>(argValue))) {
                    needsBoxing = true;
                    boxClassName = "Float";
                }
                else if (expectedType == "Bool" && std::holds_alternative<bool>(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Bool";
                }
                else if (expectedType == "String" && std::holds_alternative<std::string>(argValue)) {
                    needsBoxing = true;
                    boxClassName = "String";
                }

                if (needsBoxing) {
                    // Create boxed instance: new BoxClass(primitiveValue)
                    auto classDef = context.environment->findClass(boxClassName);
                    if (classDef) {
                        std::unordered_map<std::string, std::string> emptyBindings;
                        auto boxedInstance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef, emptyBindings);

                        // Directly set the 'value' field to avoid constructor call complexity
                        // This is safe for primitive wrappers (Int, Float, Bool, String) which just store the primitive
                        boxedInstance->setField("value", argValue);

                        argValue = boxedInstance;
                    }
                }
            }

            context.stackManager->push(argValue);

            // Register parameter by name in SharedStackFrame
            if (i < lambda->parameterNames.size()) {
                std::string paramName = lambda->parameterNames[i];
                if (!paramName.empty()) {
                    newSharedFrame->setLocal(paramName, i, argValue);
                }
            }
        }

        // Push captured variables onto stack (they become local variables after the parameters)
        // Read current values from shared frame (reference capture semantics)
        // IMPORTANT: Do NOT register them in the new SharedStackFrame - they should be accessed
        // through the parent chain to ensure we always read the latest values
        size_t capturedCount = 0;
        if (lambda->capturedFrame) {
            for (size_t i = 0; i < lambda->capturedSlots.size(); ++i) {
                size_t slot = lambda->capturedSlots[i];

                // Always use slot-based lookup to avoid name collisions
                // This allows multiple variables with the same name to coexist
                value::Value capturedValue = lambda->capturedFrame->getLocal(slot);

                context.stackManager->push(capturedValue);
                capturedCount++;
            }
        }

        // Reserve additional local variable slots if needed (for local variables like return value temporaries)
        // lambdaMetadata already looked up above for parameter type checking
        if (lambdaMetadata) {
            size_t pushedSlots = args.size() + capturedCount;  // parameters + captured
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

        // Extract simple method name from mangled name
        // methodName may be: "Calculator::add/int,int" or just "add"
        std::string simpleMethodName = methodName;

        // Remove class prefix if present
        size_t colonPos = methodName.find("::");
        if (colonPos != std::string::npos) {
            simpleMethodName = methodName.substr(colonPos + 2);
        }

        // Remove signature suffix if present
        size_t slashPos = simpleMethodName.find('/');
        if (slashPos != std::string::npos) {
            simpleMethodName = simpleMethodName.substr(0, slashPos);
        }

        // FIXED: If methodName already contains a signature (indicated by '/'),
        // use it directly instead of looking up by count only!
        // The compiler already resolved the correct overload.
        std::string qualifiedName = methodName;
        std::string definingClassName = classDef->getName();  // Default to instance class

        // If method name doesn't contain a signature, build it from resolved method
        if (methodName.find('/') == std::string::npos && methodName.find("::") == std::string::npos) {
            // Legacy path: methodName is just "add" without class or signature
            // Use findInstanceMethodInHierarchy to search only instance methods in parent classes
            auto method = classDef->findInstanceMethodInHierarchy(simpleMethodName, argCount);
            if (!method) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Instance method not found: " + methodName +
                    " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
            }

            // Find which class actually defines this method by walking up the hierarchy
            definingClassName = classDef->getName();  // Already declared above
            auto currentClass = classDef;
            while (currentClass) {
                auto localMethod = currentClass->findInstanceMethod(simpleMethodName, argCount);
                if (localMethod) {
                    definingClassName = currentClass->getName();
                    break;
                }
                currentClass = currentClass->getParentClass();
            }

            // Validate method access
            auto accessContext = createAccessContext(definingClassName, false);
            validation::AccessValidator::validateMethodAccess(simpleMethodName, method->getAccessModifier(), accessContext);

            // Build the mangled name
            auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
            qualifiedName = signature.toMangledName(definingClassName, false);
        } else {
            // New path: methodName already contains full signature like "Container::describe/int"
            // For virtual dispatch, replace the declared class with the ACTUAL object class
            // BUT preserve the signature part to maintain overload resolution

            // Extract the signature part (everything after the class name)
            std::string signaturePart = "";
            size_t classEndPos = methodName.find("::");
            if (classEndPos != std::string::npos) {
                signaturePart = methodName.substr(classEndPos + 2);  // Get "methodName/type1,type2"
            } else {
                signaturePart = methodName;  // No class prefix
            }

            auto method = classDef->findInstanceMethodInHierarchy(simpleMethodName, argCount);
            if (method) {
                // Find defining class for the actual method (virtual dispatch)
                definingClassName = classDef->getName();  // Already declared above
                auto currentClass = classDef;
                while (currentClass) {
                    auto localMethod = currentClass->findInstanceMethod(simpleMethodName, argCount);
                    if (localMethod) {
                        definingClassName = currentClass->getName();
                        break;
                    }
                    currentClass = currentClass->getParentClass();
                }

                auto accessContext = createAccessContext(definingClassName, false);
                validation::AccessValidator::validateMethodAccess(simpleMethodName, method->getAccessModifier(), accessContext);

                // Rebuild with ACTUAL class but preserve signature for overload resolution
                qualifiedName = definingClassName + "::" + signaturePart;
            }
        }

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
            std::string parentClassName = currentClass->getParentClassName();

            // Extract base class name (strip generic type parameters if present)
            // E.g., "Container<T>" -> "Container"
            std::string baseParentName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentName = parentClassName.substr(0, genericStart);
            }

            // Compare both full name and base name
            if (parentClassName == baseClass || baseParentName == baseClass) {
                return true;
            }

            // Use base name for registry lookup
            auto parentClass = context.environment->getClassRegistry()->findClass(baseParentName);
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

    // Iterator Operations Implementation
    void ObjectExecutor::handleGetIterator(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Pop the iterable collection from the stack
        value::Value collectionValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(collectionValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot get iterator from null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(collectionValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "GET_ITERATOR requires an object instance");
        }

        auto collection = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(collectionValue);

        // Call the iterator() method on the collection
        std::string methodName = "iterator";
        std::vector<value::Value> args; // No arguments for iterator()

        invokeInstanceMethod(collection, methodName, args, 0);

        // The iterator object is now on the stack (pushed by invokeInstanceMethod)
    }

    void ObjectExecutor::handleIteratorHasNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Peek at the iterator on the stack (don't pop it, we need it for next())
        value::Value iteratorValue = context.stackManager->peek();

        if (std::holds_alternative<std::nullptr_t>(iteratorValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot call hasNext() on null iterator");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_HAS_NEXT requires an iterator instance");
        }

        auto iterator = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue);

        // Call hasNext() method on the iterator
        std::string methodName = "hasNext";
        std::vector<value::Value> args;

        invokeInstanceMethod(iterator, methodName, args, 0);

        // The boolean result is now on the stack
    }

    void ObjectExecutor::handleIteratorNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Peek at the iterator on the stack
        value::Value iteratorValue = context.stackManager->peek();

        if (std::holds_alternative<std::nullptr_t>(iteratorValue)) {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot call next() on null iterator");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_NEXT requires an iterator instance");
        }

        auto iterator = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue);

        // Call next() method on the iterator
        std::string methodName = "next";
        std::vector<value::Value> args;

        invokeInstanceMethod(iterator, methodName, args, 0);

        // The next element is now on the stack
    }

    void ObjectExecutor::handleIteratorClose(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Pop the iterator from the stack
        value::Value iteratorValue = context.stackManager->pop();

        if (std::holds_alternative<std::nullptr_t>(iteratorValue)) {
            // Null iterator is OK, just return
            return;
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue)) {
            // Not an object, just ignore
            return;
        }

        auto iterator = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(iteratorValue);

        // Call close() method on the iterator (for cleanup)
        std::string methodName = "close";
        std::vector<value::Value> args;

        try {
            invokeInstanceMethod(iterator, methodName, args, 0);
            // Pop the return value (close() returns void)
            context.stackManager->pop();
        }
        catch (...) {
            // Ignore exceptions during cleanup
            // This is similar to Java's try-with-resources behavior
        }
    }
}
