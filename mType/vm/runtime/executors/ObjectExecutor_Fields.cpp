#include "ObjectExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/BoxingUtils.hpp"

using vm::runtime::utils::autoBoxPrimitive;

namespace vm::runtime
{
    void ObjectExecutor::handleGetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-212: class-targeted field read. Operands: [classNameIndex, fieldNameIndex].
        if (instr.numOperands() < 2) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD_TYPED requires 2 operands");
        }

        const std::string& className = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[1]);
        value::Value objectValue = context.stackManager->pop();

        if (value::isInt(objectValue) ||
            value::isFloat(objectValue) ||
            value::isBool(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, environment, "access field", fieldName);

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

        auto classRegistry = environment->getClassRegistry();
        auto staticClass = classRegistry->findClass(className);
        if (!staticClass) {
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

        auto fieldDef = owner->getField(fieldName);
        if (fieldDef) {
            auto accessContext = createAccessContext(owner->getName(), false);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);
        }

        size_t slot = owner->getOwnFieldIndex(fieldName);
        if (slot == SIZE_MAX) {
            value::Value fv = instance->getFieldValue(fieldName);
            context.stackManager->push(fv);
            return;
        }

        context.stackManager->push(instance->getFieldByIndex(slot));
    }

    void ObjectExecutor::handleSetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-212: class-targeted field write. Operands: [classNameIndex, fieldNameIndex].
        // Stack (top→bottom): newValue, receiver — same convention as SET_FIELD.
        if (instr.numOperands() < 2) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD_TYPED requires 2 operands");
        }

        const std::string& className = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[1]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, objectValue, context, environment, "set field", fieldName);

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

        auto classRegistry = environment->getClassRegistry();
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
}
