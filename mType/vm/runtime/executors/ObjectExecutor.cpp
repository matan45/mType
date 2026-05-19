#include "ObjectExecutor.hpp"
#include "ObjectInstanceHelper.hpp"
#include "../VirtualMachine.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/BoxingUtils.hpp"
#include "../../../value/ValueObject.hpp"

namespace vm::runtime
{
    ObjectExecutor::ObjectExecutor(ExecutionContext& ctx,
                                   std::shared_ptr<environment::Environment> env,
                                   VirtualMachine* vmPtr)
        : context(ctx)
        , environment(std::move(env))
        , vm(vmPtr)
        , instanceHelper(std::make_unique<ObjectInstanceHelper>(ctx, environment, vmPtr))
    {}

    ObjectExecutor::~ObjectExecutor() = default;

    void ObjectExecutor::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleNewObject(instr);
    }

    void ObjectExecutor::handleNewStack(const bytecode::BytecodeProgram::Instruction& instr) {
        instanceHelper->handleNewStack(instr);
    }

    void ObjectExecutor::handleNewValueObject(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.numOperands() >= 2)
        {
            const std::string& className =
                context.program->getConstantPool().getString(instr.inlineOperands[0]);
            size_t argCount = instr.inlineOperands[1];

            if (argCount == 1 &&
                value::classNameToPrimitiveTag(className) != value::PrimitiveTypeTag::NONE)
            {
                value::Value arg = context.stackManager->peek();
                value::Value boxed;
                if (utils::tryCreatePrimitiveValueObject(
                        className, std::span<const value::Value>(&arg, 1),
                        environment.get(), boxed))
                {
                    context.stackManager->pop();
                    context.stackManager->push(std::move(boxed));
                    return;
                }
            }
        }

        // Non-primitive value object construction uses the regular object path.
        // The following OBJECT_TO_VALUE bytecode converts the instance.
        instanceHelper->handleNewObject(instr);
    }

    void ObjectExecutor::handleObjectToValue(const bytecode::BytecodeProgram::Instruction& /*instr*/) {
        value::Value topValue = context.stackManager->pop();

        if (!value::isObject(topValue)) {
            // Already a ValueObject or not an object — push it back unchanged.
            context.stackManager->push(topValue);
            return;
        }

        auto instance = value::asObject(topValue);
        auto classDef = instance->getClassDefinition();

        auto valueObj = std::make_shared<value::ValueObject>(classDef);

        const auto& fieldIndexMap = classDef->getFieldIndexMap();
        for (const auto& [name, index] : fieldIndexMap) {
            valueObj->setFieldByIndex(index, instance->getFieldValue(name));
        }

        for (const auto& [param, type] : instance->getGenericTypeBindings()) {
            valueObj->setGenericTypeBinding(param, type);
        }

        context.stackManager->push(value::Value(valueObj));
    }

    // MYT-319: handleGetField / handleSetField / handleInlineSetField /
    // handleInlineGetField bodies live in ObjectExecutor.hpp for dispatch
    // inlining. handleGetFieldTyped / handleSetFieldTyped are in
    // ObjectExecutor_Fields.cpp.

    void ObjectExecutor::handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.numOperands() == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "GET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = environment->getClassRegistry();
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
        if (instr.numOperands() == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_STATIC requires operand");
        }

        const std::string& qualifiedName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value newValue = context.stackManager->pop();

        size_t colonPos = qualifiedName.find("::");
        if (colonPos == std::string::npos) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "SET_STATIC requires qualified name (ClassName::fieldName): " + qualifiedName);
        }

        std::string className = qualifiedName.substr(0, colonPos);
        std::string fieldName = qualifiedName.substr(colonPos + 2);

        auto classRegistry = environment->getClassRegistry();
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
            // Allow initialization of final fields when not yet initialized.
            if (fieldDef->isInitialized()) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Cannot assign to final static field '" + qualifiedName + "'");
            }
        }

        auto accessContext = createAccessContext(className, true);
        validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

        fieldDef->setValue(newValue);
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
            // Use the defining class from the CallFrame, not the runtime class.
            // Critical for private field access validation in inheritance.
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
                const std::string& funcName = vm->frameName(context.callStack.back());
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
        auto currentClass = environment->getClassRegistry()->findClass(derivedClass);
        while (currentClass && currentClass->hasParentClass()) {
            std::string parentClassName = currentClass->getParentClassName();

            // Strip generic type parameters if present, e.g., "Container<T>" -> "Container".
            std::string baseParentName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentName = parentClassName.substr(0, genericStart);
            }

            if (parentClassName == baseClass || baseParentName == baseClass) {
                return true;
            }

            auto parentClass = environment->getClassRegistry()->findClass(baseParentName);
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

        // Static field initialization (SET) happens in global scope.
        // Allow SET during static init (no call stack or in __script_main__).
        // MYT-197: compare interned handles (integer compare) instead of std::strings.
        bool inScriptMain = !context.callStack.empty() &&
                           context.callStack.back().functionName
                               == context.program->internFrameName("__script_main__");
        if (currentClassName.empty() && (context.callStack.empty() || inScriptMain) && isSetter) {
            isSameClass = true;
        }

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
