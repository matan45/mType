#include "ObjectExecutor.hpp"
#include "ObjectInstanceHelper.hpp"
#include "FunctionExecutor.hpp"
#include "../../MethodSignature.hpp"
#include "../../../services/ScriptAPI.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../utils/BoxingUtils.hpp"
#include <algorithm>

using vm::runtime::utils::autoBoxPrimitive;
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

    void ObjectExecutor::handleNewValueObject(const bytecode::BytecodeProgram::Instruction& instr) {
        // Value object construction uses the same mechanism as regular objects.
        // The constructor needs an ObjectInstance for 'this' (frame.thisInstance).
        // After the constructor completes, OBJECT_TO_VALUE converts the result to ValueObject.
        instanceHelper->handleNewObject(instr);
    }

    void ObjectExecutor::handleObjectToValue(const bytecode::BytecodeProgram::Instruction& instr) {
        // Convert the ObjectInstance on the stack top to a lightweight ValueObject
        value::Value topValue = context.stackManager->pop();

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(topValue)) {
            // Already a ValueObject or not an object — just push it back
            context.stackManager->push(topValue);
            return;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(topValue);
        auto classDef = instance->getClassDefinition();

        auto valueObj = std::make_shared<value::ValueObject>(classDef);

        // Copy fields from ObjectInstance to ValueObject using field index map
        const auto& fieldIndexMap = classDef->getFieldIndexMap();
        for (const auto& [name, index] : fieldIndexMap) {
            valueObj->setFieldByIndex(index, instance->getFieldValue(name));
        }

        // Copy generic type bindings
        for (const auto& [param, type] : instance->getGenericTypeBindings()) {
            valueObj->setGenericTypeBinding(param, type);
        }

        context.stackManager->push(value::Value(valueObj));
    }

    void ObjectExecutor::handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires operand");
        }

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        // Auto-box raw primitives at escape point (lazy re-boxing support)
        if (std::holds_alternative<int64_t>(objectValue) ||
            std::holds_alternative<double>(objectValue) ||
            std::holds_alternative<bool>(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, context.environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, "access field", fieldName);

        // Handle ValueObject (value types)
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(objectValue)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(objectValue);
            auto classDef = valueObj->getClassDefinition();

            auto fieldDef = classDef ? classDef->getField(fieldName) : nullptr;
            if (!fieldDef) {
                throw errors::FieldNotFoundException(fieldName, valueObj->getClassName());
            }

            // For auto-boxed primitives (Int, Float, Bool, String), skip access check
            // when accessing internal "value" field — auto-boxing is a VM-internal operation
            // and shouldn't be restricted by the calling context
            bool isAutoBoxedPrimitive = (valueObj->getClassName() == "Int" ||
                                          valueObj->getClassName() == "Float" ||
                                          valueObj->getClassName() == "Bool" ||
                                          valueObj->getClassName() == "String");
            if (!isAutoBoxedPrimitive || fieldName != "value") {
                auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
                std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();
                auto accessContext = createAccessContext(targetClassName, false);
                validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);
            }

            value::Value fieldValue = valueObj->getFieldValue(fieldName);
            context.stackManager->push(fieldValue);
            return;
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

        utils::checkNullReceiver(instr, objectValue, context, "set field", fieldName);

        // Handle ValueObject (value types) — deep copy before mutation for value semantics
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(objectValue)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(objectValue);

            // Deep copy for value semantics: always mutate a fresh copy
            auto copy = valueObj->deepCopy();

            auto classDef = copy->getClassDefinition();
            auto fieldDef = classDef ? classDef->getField(fieldName) : nullptr;
            if (!fieldDef) {
                throw errors::FieldNotFoundException(fieldName, copy->getClassName());
            }

            auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
            std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();
            auto accessContext = createAccessContext(targetClassName, true);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

            copy->setField(fieldName, newValue);

            // Push the modified copy back (caller must store it back to the variable)
            context.stackManager->push(value::Value(copy));
            return;
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
            if (fieldDef->isStatic()) {
                // Static final: use the shared FieldDefinition flag
                if (fieldDef->isInitialized()) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + fieldName + "'");
                }
            } else {
                // Instance final: check if this specific instance already has a value set
                const auto& instanceFields = instance->getAllFieldValues();
                if (instanceFields.find(fieldName) != instanceFields.end()) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + fieldName + "'");
                }
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

    void ObjectExecutor::handleInlineSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        instance->setField(fieldName, newValue);
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

        vm::profiler::ProfilerHookHelper::onFunctionEntry(frame.functionName);

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

                if (expectedType == "Int" && std::holds_alternative<int64_t>(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Int";
                }
                else if (expectedType == "Float" && (std::holds_alternative<double>(argValue) || std::holds_alternative<int64_t>(argValue))) {
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
                        if (classDef->isValueClass()) {
                            auto valueObj = std::make_shared<value::ValueObject>(classDef);
                            valueObj->setField("value", argValue);
                            argValue = value::Value(valueObj);
                        } else {
                            std::unordered_map<std::string, std::string> emptyBindings;
                            auto boxedInstance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef, emptyBindings);
                            boxedInstance->setField("value", argValue);
                            argValue = boxedInstance;
                        }
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
                    context.stackManager->push(std::monostate{});  // Sentinel for uninitialized slot
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
            // PERFORMANCE: Use cached lookup that returns method, defining class, AND qualified name
            auto lookupResult = classDef->findInstanceMethodCached(simpleMethodName, argCount);
            if (!lookupResult.method) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Instance method not found: " + methodName +
                    " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
            }

            // PERFORMANCE: Use all cached values - no string building needed!
            definingClassName = lookupResult.definingClassName;
            qualifiedName = lookupResult.qualifiedName;

            // Validate method access
            auto accessContext = createAccessContext(definingClassName, false);
            validation::AccessValidator::validateMethodAccess(simpleMethodName, lookupResult.method->getAccessModifier(), accessContext);
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

            // PERFORMANCE: Use cached lookup that returns both method AND defining class
            auto lookupResult = classDef->findInstanceMethodCached(simpleMethodName, argCount);
            if (lookupResult.method) {
                // Use cached defining class name (no duplicate hierarchy traversal!)
                definingClassName = lookupResult.definingClassName;

                auto accessContext = createAccessContext(definingClassName, false);
                validation::AccessValidator::validateMethodAccess(simpleMethodName, lookupResult.method->getAccessModifier(), accessContext);

                // Rebuild with ACTUAL class but preserve signature for overload resolution
                qualifiedName = definingClassName + "::" + signaturePart;
            }
        }

        auto funcMetadata = context.program->getFunction(qualifiedName);
        size_t targetProgramIndex = 0;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // Fallback: generic type erasure may produce a mangled name like
        // "String::equals/object" when the compiled function is "String::equals/String".
        // Search by class::method prefix to find the correct overload.
        if (!funcMetadata)
        {
            std::string prefix = definingClassName + "::" + simpleMethodName;
            for (const auto& [fname, fmeta] : context.program->getFunctions())
            {
                if (fname.rfind(prefix, 0) == 0 &&
                    (fname.size() == prefix.size() || fname[prefix.size()] == '/'))
                {
                    funcMetadata = &fmeta;
                    qualifiedName = fname;
                    break;
                }
            }
        }

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
                // Also try prefix-based fallback in each library program
                if (!funcMetadata) {
                    std::string prefix = definingClassName + "::" + simpleMethodName;
                    for (const auto& [fname, fmeta] : (*context.loadedPrograms)[i]->getFunctions()) {
                        if (fname.rfind(prefix, 0) == 0 &&
                            (fname.size() == prefix.size() || fname[prefix.size()] == '/')) {
                            funcMetadata = &fmeta;
                            qualifiedName = fname;
                            targetProgramIndex = i;
                            targetProgram = (*context.loadedPrograms)[i];
                            break;
                        }
                    }
                }
                if (funcMetadata) break;
            }
        }

        if (!funcMetadata && (simpleMethodName == "toString" || simpleMethodName == "equals" ||
                              simpleMethodName == "hashCode" || simpleMethodName == "getClass")) {
            // Native Object method fallback — when no bytecode exists for an Object method,
            // dispatch to native C++ implementations. This handles both direct Object method
            // calls and cases where overload resolution selects the Object signature (e.g.,
            // equals(null) resolving to equals(Object) when the class only has equals(SpecificType))
            auto computeHashCode = [&instance]() -> int64_t {
                std::string contentHash = instance->getContentHash();
                std::hash<std::string> hasher;
                return static_cast<int64_t>(hasher(contentHash) & 0x7FFFFFFF);
            };

            if (simpleMethodName == "toString") {
                context.stackManager->push(classDef->getName() + "@" + std::to_string(computeHashCode()));
                return;
            }
            if (simpleMethodName == "equals") {
                if (argCount >= 1) {
                    const auto& otherVal = args[0];
                    if (auto* otherPtr = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&otherVal)) {
                        if (*otherPtr) {
                            context.stackManager->push(instance->contentEquals(**otherPtr));
                        } else {
                            context.stackManager->push(false);
                        }
                    } else {
                        context.stackManager->push(false);
                    }
                } else {
                    context.stackManager->push(false);
                }
                return;
            }
            if (simpleMethodName == "hashCode") {
                context.stackManager->push(computeHashCode());
                return;
            }
            if (simpleMethodName == "getClass") {
                // MYT-42: route through ScriptAPI so language-side
                // obj.getClass() and native ScriptAPI::getClass are the
                // same code path. Builds the parameterized canonical name
                // from the instance's reified type and calls Class.forName.
                services::ScriptAPI api(context.environment, context.vm, context.program);
                context.stackManager->push(
                    api.getClass(value::Value(instance)).asValue());
                return;
            }
        }
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
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(frame);
        context.stats.functionCalls++;

        // Switch to the target program if it's from a library
        if (targetProgram != context.program) {
            context.program = targetProgram;
        }

        vm::profiler::ProfilerHookHelper::onFunctionEntry(qualifiedName);

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
                context.stackManager->push(std::monostate{});  // Sentinel for uninitialized slot
            }
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void ObjectExecutor::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // Prepare arguments from stack
        std::vector<value::Value> args = prepareMethodCallArguments(argCount);

        // Pop object and check for null (skip if compiler guarantees non-null)
        value::Value objectValue = context.stackManager->pop();

        // Auto-box raw primitives at escape point (lazy re-boxing support)
        if (std::holds_alternative<int64_t>(objectValue) ||
            std::holds_alternative<double>(objectValue) ||
            std::holds_alternative<bool>(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, context.environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, "call method", methodName);

        // Handle lambda invocation
        if (std::holds_alternative<std::shared_ptr<BytecodeLambda>>(objectValue)) {
            auto lambda = std::get<std::shared_ptr<BytecodeLambda>>(objectValue);
            invokeLambdaMethod(lambda, args, methodName);
            return;
        }

        // Handle value object method invocation
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(objectValue)) {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(objectValue);
            // Value types use the same method dispatch as regular objects
            // but 'this' is a copy (value semantics — mutations don't propagate back)
            auto classDef = valueObj->getClassDefinition();
            if (!classDef) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Value object has no class definition");
            }

            // Create a temporary ObjectInstance to serve as 'this' for the method call
            // This reuses existing method invocation infrastructure
            auto tempInstance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);
            // Copy fields from ValueObject to temporary ObjectInstance
            const auto& fieldIndexMap = classDef->getFieldIndexMap();
            for (const auto& [name, index] : fieldIndexMap) {
                if (index < valueObj->getFieldCount()) {
                    tempInstance->setField(name, valueObj->getFieldByIndex(index));
                }
            }
            // Copy generic type bindings
            for (const auto& [param, type] : valueObj->getGenericTypeBindings()) {
                tempInstance->setGenericTypeBinding(param, type);
            }

            invokeInstanceMethod(tempInstance, methodName, args, argCount);
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

        utils::checkNullReceiver(instr, collectionValue, context, "get iterator from", "collection");

        // Check if it's an array - create an ArrayIteratorHelper for it
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(collectionValue)) {
            // Get the ArrayIteratorHelper class from the class registry
            auto classRegistry = context.environment->getClassRegistry();
            auto iteratorHelperClass = classRegistry->findClass("ArrayIteratorHelper");

            if (!iteratorHelperClass) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "ArrayIteratorHelper class not found - required for array iteration");
            }

            // Create an instance of ArrayIteratorHelper with the array as constructor argument
            auto iteratorInstance = std::make_shared<runtimeTypes::klass::ObjectInstance>(iteratorHelperClass);

            // Find and invoke the constructor with 1 argument (the array)
            auto constructor = iteratorHelperClass->findConstructorByTypes({collectionValue});
            if (!constructor) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "ArrayIteratorHelper constructor not found");
            }

            // Set the array field directly (constructor would do this, but let's do it directly)
            // ArrayIteratorHelper has fields: array, index
            iteratorInstance->setField("array", collectionValue);
            iteratorInstance->setField("index", static_cast<int64_t>(0));

            // Push the iterator onto the stack
            context.stackManager->push(iteratorInstance);
            return;
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

        utils::checkNullReceiver(instr, iteratorValue, context, "call hasNext() on", "iterator");

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

        utils::checkNullReceiver(instr, iteratorValue, context, "call next() on", "iterator");

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
