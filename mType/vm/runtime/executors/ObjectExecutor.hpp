#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
#include <cstddef>
#include "../validation/AccessValidator.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../ast/AccessModifier.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../utils/BoxingUtils.hpp"
#include <span>
#include <unordered_map>
#include <memory>

namespace vm::runtime
{
    // Forward declarations
    class FunctionExecutor;
    class VirtualMachine;
    class ObjectInstanceHelper;

    /**
     * Executes object-related opcodes
     * Handles NEW_OBJECT, GET/SET_FIELD, GET/SET_STATIC, CALL_METHOD, SUPER_CONSTRUCTOR, SUPER_INVOKE
     * Manages object creation, field access, method calls, and inheritance
     */
    class ObjectExecutor
    {
    public:
        ObjectExecutor(ExecutionContext& ctx,
                       std::shared_ptr<environment::Environment> env,
                       VirtualMachine* vmPtr);
        ~ObjectExecutor();

        // Set FunctionExecutor reference for lambda-to-interface conversion
        void setFunctionExecutor(FunctionExecutor* funcExec) { functionExecutor = funcExec; }

        // Object creation
        void handleNewObject(const bytecode::BytecodeProgram::Instruction& instr);
        // MYT-134: non-escaping allocation promoted by escape-analysis — pool-borrowed raw pointer
        // tracked in the current call frame's stackObjects list.
        void handleNewStack(const bytecode::BytecodeProgram::Instruction& instr);
        void handleNewValueObject(const bytecode::BytecodeProgram::Instruction& instr);
        void handleObjectToValue(const bytecode::BytecodeProgram::Instruction& instr);

        // Field access — MYT-319: hot paths inlined for dispatch-switch inlining.
        inline void handleGetField(const bytecode::BytecodeProgram::Instruction& instr) {
            if (instr.numOperands() == 0) {
                utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires operand");
            }

            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            value::Value objectValue = context.stackManager->pop();

            // Auto-box raw primitives at escape point (lazy re-boxing support).
            // MYT-317: isAnyString covers STRING_INLINE.
            if (value::isInt(objectValue) ||
                value::isFloat(objectValue) ||
                value::isBool(objectValue) ||
                value::isAnyString(objectValue)) {
                objectValue = utils::autoBoxPrimitive(objectValue, environment);
            }

            utils::checkNullReceiver(instr, objectValue, context, environment, "access field", fieldName);

            // Handle ValueObject (value types)
            if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);
                auto classDef = valueObj->getClassDefinition();

                auto fieldDef = classDef ? classDef->getField(fieldName) : nullptr;
                if (!fieldDef) {
                    throw errors::FieldNotFoundException(fieldName, valueObj->getClassName());
                }

                // For auto-boxed primitives, skip access check when accessing internal "value" field.
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

            // MYT-208: unwrap to raw ObjectInstance* — handles both OBJECT (bridge) and STACK_OBJECT (borrowed raw).
            auto* instance = value::asObjectInstanceRaw(objectValue);

            auto fieldDef = instance->getField(fieldName);
            if (!fieldDef) {
                throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
            }

            // Validate access control — use the class that OWNS the field.
            auto classDef = instance->getClassDefinition();
            auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
            std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();

            auto accessContext = createAccessContext(targetClassName, false);

            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

            value::Value fieldValue = instance->getFieldValue(fieldName);
            context.stackManager->push(fieldValue);
        }

        inline void handleSetField(const bytecode::BytecodeProgram::Instruction& instr) {
            if (instr.numOperands() == 0) {
                utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires operand");
            }

            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            value::Value newValue = context.stackManager->pop();
            value::Value objectValue = context.stackManager->pop();

            utils::checkNullReceiver(instr, objectValue, context, environment, "set field", fieldName);

            // Handle ValueObject (value types) — deep copy before mutation for value semantics.
            if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);

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
                    if (fieldDef->isInitialized()) {
                        utils::ErrorLocationHelper::throwRuntimeError(context,
                            "Cannot assign to final field '" + fieldName + "'");
                    }
                } else {
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

            // Validate access control — use the class that OWNS the field.
            auto classDef = instance->getClassDefinition();
            auto fieldOwnerClass = classDef->getFieldOwnerInHierarchy(fieldName, classDef);
            std::string targetClassName = fieldOwnerClass ? fieldOwnerClass->getName() : classDef->getName();

            auto accessContext = createAccessContext(targetClassName, true);
            validation::AccessValidator::validateFieldAccess(fieldName, fieldDef->getAccessModifier(), accessContext);

            instance->setField(fieldName, newValue);

            // Push the value back onto the stack for chained assignments.
            context.stackManager->push(newValue);
        }

        // MYT-212: class-targeted field read/write for static binding — out-of-line
        // (deeper hierarchy walk + ClassRegistry lookup; not measurably hotter
        // than the generic path).
        void handleGetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetFieldTyped(const bytecode::BytecodeProgram::Instruction& instr);

        inline void handleInlineSetField(const bytecode::BytecodeProgram::Instruction& instr) {
            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            value::Value newValue = context.stackManager->pop();
            value::Value objectValue = context.stackManager->pop();

            // MYT-208: accept STACK_OBJECT alongside OBJECT.
            auto* instance = value::asObjectInstanceRaw(objectValue);
            instance->setField(fieldName, newValue);
        }

        inline void handleInlineGetField(const bytecode::BytecodeProgram::Instruction& instr) {
            const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            value::Value objectValue = context.stackManager->pop();

            // MYT-208: isAnyObject(...) covers both OBJECT and STACK_OBJECT.
            if (value::isAnyObject(objectValue)) {
                auto* instance = value::asObjectInstanceRaw(objectValue);
                context.stackManager->push(instance->getFieldValue(fieldName));
            } else if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);
                context.stackManager->push(valueObj->getFieldValue(fieldName));
            } else {
                // Fallback: auto-box primitive and read field
                objectValue = utils::autoBoxPrimitive(objectValue, environment);
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

        void handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr);

        // Method calls
        bool tryDispatchSpecializedCollectionCall(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr);

        // Iterator operations
        void handleGetIterator(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorHasNext(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorNext(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorClose(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-274 Phase 2: structural-equality fused opcodes for compiler-
        // synthesized hashCode / equals on int-only classes. Collapse a
        // multi-op Horner / && chain into a single bytecode instruction.
        void handleStructHashInt(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStructEqInt(const bytecode::BytecodeProgram::Instruction& instr);

        // Access context helpers (also used by InlineCacheExecutor)
        validation::AccessContext createAccessContext(
            const std::string& targetClassName,
            bool isSetter = false
        );
        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);

    private:
        ExecutionContext& context;
        std::shared_ptr<environment::Environment> environment;
        VirtualMachine* vm;
        FunctionExecutor* functionExecutor = nullptr;
        std::unique_ptr<ObjectInstanceHelper> instanceHelper;

        // Helper methods for handleCallMethod.
        // MYT-196: args passed as std::span so callers can back them with a
        // SmallArgsBuffer (no per-call heap alloc) or any vector-like.
        void invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                               std::span<const value::Value> args,
                               const std::string& methodName);
        // MYT-208: receiver passed as a Value so OBJECT (heap, shared_ptr via
        // bridge) and STACK_OBJECT (raw borrowed) can flow through the same
        // dispatch. Internally extracts the raw ObjectInstance* via
        // asObjectInstanceRaw and tag-branches the frame.thisInstance vs
        // frame.thisInstanceRaw assignment.
        void invokeInstanceMethod(const value::Value& receiverValue,
                                 const std::string& methodName,
                                 std::span<const value::Value> args,
                                 size_t argCount);

        // Value-class receiver dispatch — batch-materialises a temp
        // ObjectInstance from the ValueObject's field vector (via
        // ObjectInstance::loadFromValueObject) and delegates to the regular
        // instance path. Preserves in-method mutation semantics while
        // avoiding the per-field setField hashmap thrash of the previous
        // per-call hot loop.
        void invokeValueObjectMethod(std::shared_ptr<value::ValueObject> valueObj,
                                     const std::string& methodName,
                                     std::span<const value::Value> args,
                                     size_t argCount);

        // MYT-282: dispatch the four Object methods on array receivers.
        // Arrays have no ClassDefinition, so the regular invokeInstanceMethod
        // path doesn't apply. Handles toString / equals / hashCode /
        // getClass with simple identity-based implementations; any other
        // method name throws.
        void invokeArrayObjectMethod(const value::Value& arr,
                                     const std::string& methodName,
                                     std::span<const value::Value> args);
    };
}
