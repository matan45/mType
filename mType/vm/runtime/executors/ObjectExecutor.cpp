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
#include "../../../value/ValueShim.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../utils/BoxingUtils.hpp"
#include "../utils/MethodResolver.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
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

    void ObjectExecutor::handleNewStack(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleNewStack(instr);
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

        if (!value::isObject(topValue)) {
            // Already a ValueObject or not an object — just push it back
            context.stackManager->push(topValue);
            return;
        }

        auto instance = value::asObject(topValue);
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

        // Auto-box raw primitives at escape point (lazy re-boxing support).
        // INVOKE_STRING_CONCAT can leave a raw STRING / INTERNED_STRING on the
        // stack; route those through the same boxing path as int/float/bool.
        if (value::isInt(objectValue) ||
            value::isFloat(objectValue) ||
            value::isBool(objectValue) ||
            value::isString(objectValue) ||
            value::isInternedString(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, context.environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, "access field", fieldName);

        // Handle ValueObject (value types)
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
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

        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires an object instance");
        }

        // MYT-208: unwrap to raw ObjectInstance* — handles both OBJECT (bridge)
        // and STACK_OBJECT (borrowed raw) tags uniformly.
        auto* instance = value::asObjectInstanceRaw(objectValue);

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

    void ObjectExecutor::handleGetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-212: class-targeted field read. Operands: [classNameIndex, fieldNameIndex].
        if (instr.operands.size() < 2) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD_TYPED requires 2 operands");
        }

        const std::string& className = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[1]);
        value::Value objectValue = context.stackManager->pop();

        if (value::isInt(objectValue) ||
            value::isFloat(objectValue) ||
            value::isBool(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, context.environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, "access field", fieldName);

        // MYT-225: value-class receivers (e.g. `other.x` where `other` is a
        // value-class parameter) appear with tag VALUE_OBJECT, not OBJECT.
        // Resolve directly through the value object's own class — value classes
        // cannot extend, so the static-binding hierarchy walk is moot.
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
            auto classDef = valueObj->getClassDefinition();
            auto fieldDef = classDef ? classDef->getField(fieldName) : nullptr;
            if (!fieldDef) {
                throw errors::FieldNotFoundException(fieldName, valueObj->getClassName());
            }

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

            context.stackManager->push(valueObj->getFieldValue(fieldName));
            return;
        }

        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD_TYPED requires an object instance");
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);

        auto classRegistry = context.environment->getClassRegistry();
        auto staticClass = classRegistry->findClass(className);
        if (!staticClass) {
            // Static class not found at runtime; fall back to dynamic lookup.
            value::Value fv = instance->getFieldValue(fieldName);
            context.stackManager->push(fv);
            return;
        }

        // Walk the named class's hierarchy for the closest ancestor that
        // declares the field, then read its OWN slot. This is the static-
        // binding semantics: a Parent-typed receiver sees Parent's slot,
        // even when the runtime instance is a Child that shadows the field.
        auto owner = staticClass->getFieldOwnerInHierarchy(fieldName, staticClass);
        if (!owner) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        // Access control still keys on the owning class.
        auto fieldDef = owner->getField(fieldName);
        if (fieldDef) {
            auto accessContext = createAccessContext(owner->getName(), false);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);
        }

        size_t slot = owner->getOwnFieldIndex(fieldName);
        if (slot == SIZE_MAX) {
            // Static fields and other non-indexed fields fall back to dynamic lookup.
            value::Value fv = instance->getFieldValue(fieldName);
            context.stackManager->push(fv);
            return;
        }

        context.stackManager->push(instance->getFieldByIndex(slot));
    }

    void ObjectExecutor::handleSetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-212: class-targeted field write. Operands: [classNameIndex, fieldNameIndex].
        // Stack (top→bottom): newValue, receiver — same convention as SET_FIELD.
        if (instr.operands.size() < 2) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD_TYPED requires 2 operands");
        }

        const std::string& className = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[1]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, objectValue, context, "set field", fieldName);

        // MYT-225: value-class receivers — deep copy before mutation for value
        // semantics. Mirrors handleSetField's value-object branch plus the
        // instance-final guard from the reference-class path below.
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
            auto copy = valueObj->deepCopy();
            auto classDef = copy->getClassDefinition();
            auto fieldDef = classDef ? classDef->getField(fieldName) : nullptr;
            if (!fieldDef) {
                throw errors::FieldNotFoundException(fieldName, copy->getClassName());
            }

            if (fieldDef->isFinal() && !fieldDef->isStatic()) {
                size_t finalSlot = classDef->getOwnFieldIndex(fieldName);
                if (finalSlot != SIZE_MAX &&
                    !value::isVoid(copy->getFieldByIndex(finalSlot))) {
                    utils::ErrorLocationHelper::throwRuntimeError(context,
                        "Cannot assign to final field '" + fieldName + "'");
                }
            }

            auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
            std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();
            auto accessContext = createAccessContext(targetClassName, true);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

            copy->setField(fieldName, newValue);
            context.stackManager->push(value::Value(copy));
            return;
        }

        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD_TYPED requires an object instance");
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);

        auto classRegistry = context.environment->getClassRegistry();
        auto staticClass = classRegistry->findClass(className);
        if (!staticClass) {
            instance->setField(fieldName, newValue);
            context.stackManager->push(newValue);
            return;
        }

        auto owner = staticClass->getFieldOwnerInHierarchy(fieldName, staticClass);
        if (!owner) {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        auto fieldDef = owner->getField(fieldName);
        if (fieldDef && fieldDef->isFinal()) {
            // Mirror SET_FIELD's instance-final guard: reject if the slot is
            // already non-void on this instance. Inline initializers are the
            // first write into the slot and must succeed.
            if (!fieldDef->isStatic()) {
                size_t finalSlot = owner->getOwnFieldIndex(fieldName);
                if (finalSlot != SIZE_MAX) {
                    if (!value::isVoid(instance->getFieldByIndex(finalSlot))) {
                        utils::ErrorLocationHelper::throwRuntimeError(context,
                            "Cannot assign to final field '" + fieldName + "'");
                    }
                }
            }
        }

        if (fieldDef) {
            auto accessContext = createAccessContext(owner->getName(), true);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);
        }

        size_t slot = owner->getOwnFieldIndex(fieldName);
        if (slot == SIZE_MAX) {
            instance->setField(fieldName, newValue);
        } else {
            instance->setFieldByIndex(slot, newValue);
        }

        context.stackManager->push(newValue);
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
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);

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

        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires an object instance");
        }

        // MYT-208: raw unwrap supports OBJECT and STACK_OBJECT tags.
        auto* instance = value::asObjectInstanceRaw(objectValue);

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
                size_t idx = instance->getClassDefinition()->getFieldIndex(fieldName);
                if (idx != SIZE_MAX)
                {
                    if (!value::isVoid(instance->getFieldByIndex(idx)))
                    {
                        utils::ErrorLocationHelper::throwRuntimeError(context,
                            "Cannot assign to final field '" + fieldName + "'");
                    }
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

        // MYT-208: accept STACK_OBJECT alongside OBJECT.
        auto* instance = value::asObjectInstanceRaw(objectValue);
        instance->setField(fieldName, newValue);
    }

    void ObjectExecutor::handleInlineGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        // MYT-208: isAnyObject(...) covers both OBJECT and STACK_OBJECT;
        // asObjectInstanceRaw unwraps both into a non-owning ObjectInstance*.
        if (value::isAnyObject(objectValue)) {
            auto* instance = value::asObjectInstanceRaw(objectValue);
            context.stackManager->push(instance->getFieldValue(fieldName));
        } else if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
            context.stackManager->push(valueObj->getFieldValue(fieldName));
        } else {
            // Fallback: auto-box primitive and read field
            objectValue = autoBoxPrimitive(objectValue, context.environment);
            if (value::isAnyObject(objectValue)) {
                auto* instance = value::asObjectInstanceRaw(objectValue);
                context.stackManager->push(instance->getFieldValue(fieldName));
            } else if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);
                context.stackManager->push(valueObj->getFieldValue(fieldName));
            } else {
                throw errors::RuntimeException("INLINE_GET_FIELD: cannot read field '" + fieldName + "' from non-object");
            }
        }
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

    void ObjectExecutor::invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                                           std::span<const value::Value> args,
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
        // Use the lambda's unique function name (e.g., __lambda_0) for metadata/exception table lookup.
        // MYT-197: 4-byte handle copy; fall back to an interned display name
        // only for lambdas created outside handleLambda.
        if (lambda->functionName != bytecode::INVALID_FN_HANDLE) {
            frame.functionName = lambda->functionName;
        } else {
            std::string fallback = lambda->creatingClassName.empty()
                ? "<lambda>"
                : lambda->creatingClassName + "::<lambda>";
            frame.functionName = context.program->internFrameName(fallback);
        }
        frame.thisInstance = lambda->capturedThis;  // Restore captured 'this'
        frame.originatingLambda = lambda;  // Store lambda reference for variable access
        frame.definingClassName = lambda->creatingClassName;  // Set creating class for access control

        context.pushCallFrame(std::move(frame));

        const std::string& frameNameStr = context.program->getFrameName(frame.functionName);
        vm::profiler::ProfilerHookHelper::onFunctionEntry(frameNameStr);

        // Notify debugger of lambda entry
        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
            } else {
                // Fallback: use lambda start location if current instruction has no location
                auto lambdaStartLoc = context.program->getSourceLocation(lambdaStart);
                if (lambdaStartLoc) {
                    errors::SourceLocation errorsLoc(lambdaStartLoc->filename, lambdaStartLoc->line, lambdaStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errors::SourceLocation());
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

        // Get lambda metadata for parameter type information.
        // MYT-197: O(1) handle lookup.
        auto* lambdaMetadata = context.program->getFunctionMeta(lambda->functionName);

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

                if (expectedType == "Int" && value::isInt(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Int";
                }
                else if (expectedType == "Float" && (value::isFloat(argValue) || value::isInt(argValue))) {
                    needsBoxing = true;
                    boxClassName = "Float";
                }
                else if (expectedType == "Bool" && value::isBool(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Bool";
                }
                else if (expectedType == "String" && value::isString(argValue)) {
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
                            auto boxedInstance = value::ObjectInstancePool::getInstance().acquire(classDef, emptyBindings);
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

    void ObjectExecutor::invokeInstanceMethod(const value::Value& receiverValue,
                                             const std::string& methodName,
                                             std::span<const value::Value> args,
                                             size_t argCount) {
        // MYT-208: tag-aware unwrap. raw is the only handle used for data
        // access (getClassDefinition, getField, contentEquals, etc.); it is
        // valid for both OBJECT and STACK_OBJECT receivers. The original
        // Value is preserved in `receiverValue` so we can push it back as
        // `this` (preserving the tag) and so we can branch on tag when
        // populating the new CallFrame.
        auto* instance = value::asObjectInstanceRaw(receiverValue);
        auto classDef = instance->getClassDefinition();

        // Extract simple method name from mangled name
        // methodName may be: "Calculator::add/int,int" or just "add"
        std::string simpleMethodName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(methodName);

        // FIXED: If methodName already contains a signature (indicated by '/'),
        // use it directly instead of looking up by count only!
        // The compiler already resolved the correct overload.
        std::string qualifiedName = methodName;
        std::string definingClassName = classDef->getName();  // Default to instance class

        // If method name doesn't contain a signature, build it from resolved method
        if (methodName.find('/') == std::string::npos && methodName.find("::") == std::string::npos) {
            // Legacy path: methodName is just "add" without class or signature
            // PERFORMANCE: Use cached lookup that returns method, defining class, AND qualified name
            auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
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

            // PERFORMANCE: Use cached lookup that returns both method AND defining class.
            // Signed call names must bind the exact overload selected by the compiler.
            auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
            if (lookupResult.method) {
                // Use cached defining class name (no duplicate hierarchy traversal!)
                definingClassName = lookupResult.definingClassName;

                auto accessContext = createAccessContext(definingClassName, false);
                validation::AccessValidator::validateMethodAccess(simpleMethodName, lookupResult.method->getAccessModifier(), accessContext);

                qualifiedName = lookupResult.qualifiedName;
            }
        }

        auto resolution = utils::MethodResolver::resolve(
            qualifiedName, definingClassName, simpleMethodName, context);
        auto funcMetadata = resolution.funcMetadata;
        size_t targetProgramIndex = resolution.programIndex;
        const bytecode::BytecodeProgram* targetProgram = resolution.program;
        qualifiedName = resolution.qualifiedName;

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
                // Default Object.equals() — content-based equality. Iterates
                // the declared instance fields and compares element-wise via
                // ObjectInstance::contentEqualsImpl. `==` stays reference-only
                // (see objectIdentity.mt); user overrides customize further.
                // Per implicitObjectInheritance.mt and objectEqualsNull.mt,
                // two instances of the same class with identical field values
                // compare equal.
                if (argCount >= 1) {
                    const auto& otherVal = args[0];
                    // MYT-208: also accept STACK_OBJECT arguments.
                    if (value::isAnyObject(otherVal)) {
                        auto* otherPtr = value::asObjectInstanceRaw(otherVal);
                        if (otherPtr) {
                            context.stackManager->push(instance->contentEquals(*otherPtr));
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
                // MYT-208: pass the original receiver Value so getClass works
                // for both OBJECT and STACK_OBJECT receivers without re-boxing.
                context.stackManager->push(
                    api.getClass(receiverValue).asValue());
                return;
            }
        }
        if (!funcMetadata) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }

        // Convert lambda arguments to interface implementations if needed.
        // Copy span into a local vector so we can mutate slots for auto-boxing.
        std::vector<value::Value> modifiedArgs(args.begin(), args.end());
        if (functionExecutor) {
            functionExecutor->convertLambdaArgumentsToInterfaces(modifiedArgs, funcMetadata->parameterTypes);
        }

        size_t frameBase = context.stackManager->size();
        // MYT-208: push the original receiver Value (preserves OBJECT vs
        // STACK_OBJECT tag) so the callee's LOAD_LOCAL slot 0 reads the right
        // tag and stays refcount-free for STACK_OBJECT.
        context.stackManager->push(receiverValue);

        for (size_t i = 0; i < argCount; ++i) {
            context.stackManager->push(modifiedArgs[i]);
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = frameBase;
        // MYT-197: intern on the target program so the handle is resolvable
        // by the same program that owns the FunctionMetadata.
        frame.functionName = targetProgram->internFrameName(qualifiedName);
        // MYT-208: tag-branch `this` ownership. STACK_OBJECT routes the raw
        // pointer through thisInstanceRaw (lifetime owned by caller frame's
        // stackObjects); OBJECT keeps the shared_ptr in thisInstance.
        if (value::isStackObject(receiverValue)) {
            frame.thisInstanceRaw = instance;
        } else {
            frame.thisInstance = value::asObject(receiverValue);
        }
        frame.definingClassName = definingClassName;  // Store the class that defines this method
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(std::move(frame));
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

    void ObjectExecutor::invokeValueObjectMethod(
        std::shared_ptr<value::ValueObject> valueObj,
        const std::string& methodName,
        std::span<const value::Value> args,
        size_t argCount)
    {
        auto classDef = valueObj->getClassDefinition();
        if (!classDef) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Value object has no class definition");
        }

        // Materialise the receiver as a temp ObjectInstance so the method body
        // gets the in-place mutation semantics that value-class tests rely on
        // (see inline_value_object_write_skip.mt — `this.n = this.n + 1` must
        // be visible within the call). The previous hot loop ran setField per
        // declared field; loadFromValueObject batch-copies the vector and
        // syncs fieldValues in one pass — no GC write barrier, no per-field
        // hashmap probes, no gcRegistered branch.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(classDef);
        tempInstance->loadFromValueObject(*valueObj);

        // MYT-208: invokeInstanceMethod takes a Value receiver now. Wrap the
        // temp shared_ptr in an OBJECT-tagged Value (always heap, never
        // stack-promoted — value-class temp instance has unbounded lifetime
        // tied to the shared_ptr).
        invokeInstanceMethod(value::Value(tempInstance), methodName, args, argCount);
    }

    void ObjectExecutor::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // MYT-196: pop arguments into a small-buffer-optimized scratch buffer.
        // Buffer outlives the invokeLambdaMethod / invokeInstanceMethod call,
        // so the span handed to them remains valid.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // Pop object and check for null (skip if compiler guarantees non-null)
        value::Value objectValue = context.stackManager->pop();

        // Auto-box raw primitives at escape point (lazy re-boxing support).
        // INVOKE_STRING_CONCAT can leave a raw STRING / INTERNED_STRING on the
        // stack; route those through the same boxing path as int/float/bool.
        if (value::isInt(objectValue) ||
            value::isFloat(objectValue) ||
            value::isBool(objectValue) ||
            value::isString(objectValue) ||
            value::isInternedString(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, context.environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, "call method", methodName);

        // Handle lambda invocation
        if (value::isLambda(objectValue)) {
            auto lambda = value::asLambda(objectValue);
            invokeLambdaMethod(lambda, args.span(), methodName);
            return;
        }

        // Value-class receiver — materialise via the batch-load helper so the
        // per-call cost is a single vector copy plus one synchronised
        // hashmap fill, instead of the N × setField thrash the previous
        // inline temp-materialisation did.
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
            invokeValueObjectMethod(valueObj, methodName, args.span(), argCount);
            return;
        }

        // Handle regular instance method invocation.
        // MYT-208: accept STACK_OBJECT receivers — invokeInstanceMethod
        // tag-branches the frame.thisInstance / thisInstanceRaw assignment
        // and pushes the original Value onto the stack as `this`.
        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwUserException(context,
                "RuntimeException",
                "CALL_METHOD requires an object instance or lambda");
        }

        invokeInstanceMethod(objectValue, methodName, args.span(), argCount);
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
            // Fallback to instance class if defining class not set (e.g., for constructors).
            // MYT-208: also accept stack-promoted `this` (NEW_STACK ctor frames).
            if (auto* rawThis = context.callStack.back().getThisInstanceRaw()) {
                return rawThis->getClassDefinition()->getName();
            } else {
                // Fallback: extract class name from function name (static methods).
                // MYT-197: resolve the interned handle before splitting.
                const std::string& funcName = context.frameName(context.callStack.back());
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
        // Allow SET during static initialization (either no call stack or in __script_main__).
        // MYT-197: compare interned handles (integer compare) instead of std::strings.
        bool inScriptMain = !context.callStack.empty() &&
                           context.callStack.back().functionName
                               == context.program->internFrameName("__script_main__");
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
        if (value::isNativeArray(collectionValue)) {
            // Get the ArrayIteratorHelper class from the class registry
            auto classRegistry = context.environment->getClassRegistry();
            auto iteratorHelperClass = classRegistry->findClass("ArrayIteratorHelper");

            if (!iteratorHelperClass) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "ArrayIteratorHelper class not found - required for array iteration");
            }

            // Create an instance of ArrayIteratorHelper with the array as constructor argument
            auto iteratorInstance = value::ObjectInstancePool::getInstance().acquire(iteratorHelperClass);

            // Find and invoke the constructor with 1 argument (the array).
            // Local array lets us hand a span without allocating a vector.
            value::Value oneArg[] = { collectionValue };
            auto constructor = iteratorHelperClass->findConstructorByTypes(
                std::span<const value::Value>(oneArg, 1));
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

        if (!value::isAnyObject(collectionValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "GET_ITERATOR requires an object instance");
        }

        // MYT-208: pass the receiver Value directly so OBJECT and STACK_OBJECT
        // collections both dispatch correctly.
        std::string methodName = "iterator";
        invokeInstanceMethod(collectionValue, methodName, std::span<const value::Value>{}, 0);

        // The iterator object is now on the stack (pushed by invokeInstanceMethod)
    }

    void ObjectExecutor::handleIteratorHasNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-156: pop the iterator. The for-each compiler emits a fresh
        // LOAD_LOCAL iter before each iterator op, so popping here keeps the
        // operand stack balanced (peeking leaked one slot per iteration and
        // blocked OSR with OPERAND_STACK_NOT_EMPTY).
        value::Value iteratorValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, iteratorValue, context, "call hasNext() on", "iterator");

        if (!value::isAnyObject(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_HAS_NEXT requires an iterator instance");
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "hasNext";
        invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);

        // The boolean result is now on the stack
    }

    void ObjectExecutor::handleIteratorNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-156: pop the iterator (see handleIteratorHasNext).
        value::Value iteratorValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, iteratorValue, context, "call next() on", "iterator");

        if (!value::isAnyObject(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_NEXT requires an iterator instance");
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "next";
        invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);

        // The next element is now on the stack
    }

    void ObjectExecutor::handleIteratorClose(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Pop the iterator from the stack
        value::Value iteratorValue = context.stackManager->pop();

        if (value::isNullType(iteratorValue)) {
            // Null iterator is OK, just return
            return;
        }

        if (!value::isAnyObject(iteratorValue)) {
            // Not an object, just ignore
            return;
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "close";
        try {
            invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);
            // Pop the return value (close() returns void)
            context.stackManager->pop();
        }
        catch (...) {
            // Ignore exceptions during cleanup
            // This is similar to Java's try-with-resources behavior
        }
    }

    // MYT-274 Phase 2: structural-equality fused-opcode handlers.
    //
    // STRUCT_HASH_INT operand layout: [fieldCount, slot0, slot1, ...].
    // Pops `this`, reads each int field by index, computes Horner-style hash,
    // pushes int. Mirrors what the synthesized Horner expression body does
    // but in a single dispatch with no operand-stack churn between fields.
    void ObjectExecutor::handleStructHashInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty())
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_HASH_INT requires fieldCount operand");
        }
        const size_t fieldCount = static_cast<size_t>(instr.operands[0]);
        if (instr.operands.size() < 1 + fieldCount)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_HASH_INT operand list truncated");
        }

        value::Value receiverValue = context.stackManager->pop();
        utils::checkNullReceiver(instr, receiverValue, context, "compute hashCode of", "this");

        int64_t h = 1;

        // Resolve field reader once based on receiver kind. The hot path is
        // ObjectInstance (heap), with ValueObject the secondary case for
        // value-class receivers (none in Phase 1's gate, but cheap to
        // support for future-proofing).
        auto* objRaw = value::asObjectInstanceRaw(receiverValue);
        if (objRaw && objRaw->hasFieldVector())
        {
            for (size_t i = 0; i < fieldCount; ++i)
            {
                const size_t slot = static_cast<size_t>(instr.operands[1 + i]);
                const value::Value& fv = objRaw->getFieldByIndex(slot);
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }
        else if (value::isValueObject(receiverValue))
        {
            auto vobj = value::asValueObject(receiverValue);
            for (size_t i = 0; i < fieldCount; ++i)
            {
                const size_t slot = static_cast<size_t>(instr.operands[1 + i]);
                const value::Value& fv = vobj->getFieldByIndex(slot);
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }

        context.stackManager->push(value::Value(h));
    }

    // STRUCT_EQ_INT operand layout: [ownerClassNameIdx, fieldCount, slot0, slot1, ...].
    // Pops `other` (Object), pops `this`. Push false if other is null or
    // not an instance of ownerClass. Otherwise compare each int field
    // pairwise; push false on mismatch, true when all match.
    void ObjectExecutor::handleStructEqInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.size() < 2)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_EQ_INT requires owner-class name and fieldCount");
        }
        const std::string& ownerClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const size_t fieldCount = static_cast<size_t>(instr.operands[1]);
        if (instr.operands.size() < 2 + fieldCount)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_EQ_INT operand list truncated");
        }

        value::Value otherValue = context.stackManager->pop();
        value::Value thisValue = context.stackManager->pop();

        // Null other → not equal. Class-mismatch → not equal. The synthesized
        // body's `isClassOf <Owner>` guard collapses into this check.
        if (value::isNullType(otherValue))
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        auto* otherRaw = value::asObjectInstanceRaw(otherValue);
        auto* thisRaw = value::asObjectInstanceRaw(thisValue);
        if (!otherRaw || !thisRaw || !otherRaw->hasFieldVector() || !thisRaw->hasFieldVector())
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        // Class compatibility: `other` must be an instance of ownerClassName
        // (the class on which the synthesized equals lives). isSubclass
        // covers the same-class case (subclass-of-self == true).
        const std::string& otherClassName = otherRaw->getClassDefinition()->getName();
        if (otherClassName != ownerClassName && !isSubclass(otherClassName, ownerClassName))
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        for (size_t i = 0; i < fieldCount; ++i)
        {
            const size_t slot = static_cast<size_t>(instr.operands[2 + i]);
            const value::Value& a = thisRaw->getFieldByIndex(slot);
            const value::Value& b = otherRaw->getFieldByIndex(slot);
            const int64_t ai = value::isInt(a) ? value::asInt(a) : 0;
            const int64_t bi = value::isInt(b) ? value::asInt(b) : 0;
            if (ai != bi)
            {
                context.stackManager->push(value::Value(false));
                return;
            }
        }

        context.stackManager->push(value::Value(true));
    }
}
