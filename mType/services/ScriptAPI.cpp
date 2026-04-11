#include "ScriptAPI.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/TypeConversionException.hpp"
#include "../errors/ReturnException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/runtime/executors/TypeExecutor.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtime/EventLoop.hpp"
#include "../value/PromiseValue.hpp"
#include "../value/NativeArray.hpp"
#include "../reflection/ReflectionHandle.hpp"
#include "../types/UnifiedType.hpp"
#include <iostream>

namespace services
{
    namespace
    {
        /**
         * Centralized unwrap for ObjectInstance values — the single chokepoint
         * that replaces the `std::get<std::shared_ptr<ObjectInstance>>` pattern
         * across the public ScriptAPI surface (MYT-42).
         *
         * Throws ObjectException tagged with `apiName` when the value does not
         * hold an ObjectInstance, so error messages tell you which API call
         * rejected the argument.
         */
        std::shared_ptr<runtimeTypes::klass::ObjectInstance>
        asObjectInstance(const value::Value& v, const char* apiName)
        {
            if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v))
            {
                throw errors::ObjectException(
                    "Value is not an object", "", apiName);
            }
            auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
            if (!instance)
            {
                throw errors::ObjectException(
                    "Value is a null object", "", apiName);
            }
            return instance;
        }
    }

    ScriptAPI::ScriptAPI(std::shared_ptr<environment::Environment> env,
                         vm::runtime::VirtualMachine* virtualMachine,
                         const vm::bytecode::BytecodeProgram* bytecodeProgram)
        : environment(env), vm(virtualMachine), program(bytecodeProgram)
    {
    }

    ScriptAPI::~ScriptAPI() = default;

    void ScriptAPI::setBytecodeProgram(const vm::bytecode::BytecodeProgram* bytecodeProgram)
    {
        program = bytecodeProgram;
    }

    value::Value ScriptAPI::callFunction(const std::string& functionName, const std::vector<value::Value>& args)
    {
        // Try to find a native function first
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
        {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc)
            {
                return nativeFunc(args);
            }
        }

        // Otherwise use VM to invoke the function
        if (vm && program)
        {
            return vm->executeFunction(functionName, args);
        }

        throw errors::MethodNotFoundException(functionName, "", __FUNCTION__);
    }

    value::Value ScriptAPI::callMethod(const value::Value& object,
                                       const std::string& methodName,
                                       const std::vector<value::Value>& args)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::callMethod");

        // Use VM for method invocation
        if (vm)
        {
            // Check if method is async — if so, schedule via EventLoop
            auto classDef = instance->getClassDefinition();
            auto method = classDef->findInstanceMethodInHierarchy(methodName, args.size());
            if (method && method->getIsAsync())
            {
                auto* eventLoop = vm->ensureEventLoop();
                auto vmShared = vm->shared_from_this();

                size_t taskId = eventLoop->scheduleTask(
                    [vmShared, inst = instance, methodName, args]() -> value::Value {
                        return vmShared->invokeMethod(inst, methodName, args);
                    }
                );
                eventLoop->setTaskVM(taskId, vmShared);

                // Return the task's result promise so caller can check completion
                auto task = eventLoop->getTask(taskId);
                if (task && task->resultPromise)
                {
                    return value::Value(task->resultPromise);
                }
                return value::Value(std::monostate{});
            }

            return vm->invokeMethod(instance, methodName, args);
        }

        throw errors::MethodNotFoundException(methodName, instance->getClassDefinition()->getName());
    }

    value::Value ScriptAPI::callStaticMethod(const std::string& className,
                                             const std::string& methodName,
                                             const std::vector<value::Value>& args)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        // Use VM for static method invocation
        if (vm)
        {
            // Check if method is async — if so, schedule via EventLoop
            auto method = classDef->findStaticMethod(methodName, args.size());
            if (method && method->getIsAsync())
            {
                auto* eventLoop = vm->ensureEventLoop();
                auto vmShared = vm->shared_from_this();

                size_t taskId = eventLoop->scheduleTask(
                    [vmShared, className, methodName, args]() -> value::Value {
                        return vmShared->invokeStaticMethod(className, methodName, args);
                    }
                );
                eventLoop->setTaskVM(taskId, vmShared);

                auto task = eventLoop->getTask(taskId);
                if (task && task->resultPromise)
                {
                    return value::Value(task->resultPromise);
                }
                return value::Value(std::monostate{});
            }

            return vm->invokeStaticMethod(className, methodName, args);
        }

        throw errors::MethodNotFoundException(methodName, className);
    }

    value::Value ScriptAPI::callLambda(const value::Value& lambda,
                                       const std::vector<value::Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<vm::runtime::BytecodeLambda>>(lambda))
        {
            throw errors::ObjectException("Cannot invoke non-lambda value", "", __FUNCTION__);
        }

        auto lambdaPtr = std::get<std::shared_ptr<vm::runtime::BytecodeLambda>>(lambda);

        if (vm)
        {
            return vm->invokeLambda(lambdaPtr, args);
        }

        throw errors::RuntimeException("Cannot invoke lambda: VM not available");
    }

    value::Value ScriptAPI::getStaticField(const std::string& className, const std::string& fieldName)
    {
        // Use VM for static field access
        if (vm)
        {
            return vm->getStaticField(className, fieldName);
        }

        // Fall back to direct field access if VM not available
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        return field->getValue();
    }

    void ScriptAPI::setStaticField(const std::string& className,
                                   const std::string& fieldName,
                                   const value::Value& value)
    {
        // Use VM for static field access
        if (vm)
        {
            vm->setStaticField(className, fieldName, value);
            return;
        }

        // Fall back to direct field access if VM not available
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (field->isFinal())
        {
            throw errors::FinalModificationException(fieldName, className);
        }

        field->setValue(value);
    }

    value::Value ScriptAPI::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        return varDef->getValue();
    }

    void ScriptAPI::setVariable(const std::string& variableName, const value::Value& value)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        if (varDef->isFinal())
        {
            throw errors::FinalModificationException(variableName);
        }

        varDef->setValue(value);
    }

    value::Value ScriptAPI::getField(const value::Value& object, const std::string& fieldName)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::getField");

        // Use VM for field access
        if (vm)
        {
            return vm->getField(instance, fieldName);
        }

        // Fall back to direct field access if VM not available
        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        return fieldDef->getValue();
    }

    void ScriptAPI::setField(const value::Value& object,
                            const std::string& fieldName,
                            const value::Value& value)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::setField");

        // Use VM for field access
        if (vm)
        {
            vm->setField(instance, fieldName, value);
            return;
        }

        // Fall back to direct field access if VM not available
        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::FinalModificationException(fieldName, instance->getClassDefinition()->getName());
        }

        fieldDef->setValue(value);
    }

    value::Value ScriptAPI::createObject(const std::string& className,
                                         const std::vector<value::Value>& constructorArgs)
    {
        // Use VM for object creation
        if (vm)
        {
            return vm->createObject(className, constructorArgs);
        }

        throw errors::ClassNotFoundException(className);
    }

    bool ScriptAPI::isObjectOfClass(const value::Value& object, const std::string& className)
    {
        // Silent-false for non-objects is the documented behavior of this
        // utility — keep it distinct from the throw-on-mismatch helpers.
        // Uses get_if so this file still passes the hygiene grep (no
        // `std::get<std::shared_ptr<ObjectInstance>>` outside asObjectInstance).
        auto* instancePtr =
            std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&object);
        if (!instancePtr || !*instancePtr)
        {
            return false;
        }
        auto classDef = (*instancePtr)->getClassDefinition();
        return classDef && classDef->getName() == className;
    }

    std::string ScriptAPI::getObjectClassName(const value::Value& object)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::getObjectClassName");
        return instance->getClassDefinition()->getName();
    }

    // ================================================================
    // MYT-42 — generic type metadata / Class FFI
    // ================================================================

    api::Class ScriptAPI::getClass(const value::Value& object)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::getClass");

        // Reconstruct the canonical parameterized name using the same
        // helper the bytecode INSTANCEOF op uses — this guarantees the
        // string we pass to Class.forName lines up with what the language
        // side would produce (e.g. "Box<Int>" with ", " spacing).
        std::string canonicalName =
            vm::runtime::TypeExecutor::reconstructObjectFullType(instance);
        if (canonicalName.empty())
        {
            canonicalName = instance->getClassDefinition()->getName();
        }

        // Route through the same reflection path as language-side
        // Class.forName — same ReflectionHandleRegistry, same interning,
        // same handle identity.
        value::Value raw = callStaticMethod(
            "Class", "forName", { value::Value(canonicalName) });
        return api::Class(std::move(raw));
    }

    api::Class ScriptAPI::classOf(const std::string& typeName)
    {
        value::Value raw = callStaticMethod(
            "Class", "forName", { value::Value(typeName) });
        return api::Class(std::move(raw));
    }

    std::vector<api::Class> ScriptAPI::getGenericArguments(const value::Value& object)
    {
        // Route through the language surface: Class.getTypeArguments()
        // returns Class[] via the same __reflect_getTypeArguments native
        // that powers the language path, so identity is preserved.
        api::Class cls = getClass(object);
        value::Value argsArray = callMethod(cls.asValue(), "getTypeArguments", {});

        // Class.getTypeArguments() always returns a Class[] — at the VM
        // level a shared_ptr<NativeArray> with object elements. Anything
        // else is an internal invariant violation and must fail loudly
        // rather than silently return an empty vector.
        if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(argsArray))
        {
            throw errors::ObjectException(
                "Class.getTypeArguments() returned an unexpected value shape "
                "(expected NativeArray)",
                "Class", "ScriptAPI::getGenericArguments");
        }
        auto array = std::get<std::shared_ptr<value::NativeArray>>(argsArray);
        if (!array)
        {
            throw errors::ObjectException(
                "Class.getTypeArguments() returned a null array",
                "Class", "ScriptAPI::getGenericArguments");
        }

        std::vector<api::Class> result;
        result.reserve(array->size());
        for (size_t i = 0; i < array->size(); ++i)
        {
            // Each element must be a Class ObjectInstance — the api::Class
            // ctor throws with a clear message if that invariant is ever
            // violated (e.g. if Class.mt is ever refactored to return
            // something other than Class[]).
            result.emplace_back(array->get(i));
        }
        return result;
    }

    bool ScriptAPI::isInstanceOf(const value::Value& object,
                                 const std::string& fullyParameterizedType)
    {
        // Shared code path with bytecode INSTANCEOF — same routine in
        // TypeExecutor handles primitives, parameterized objects, and the
        // interface hierarchy walk.
        return vm::runtime::TypeExecutor::checkInstanceOfByName(
            object, fullyParameterizedType, environment);
    }

    bool ScriptAPI::isInstanceOf(const value::Value& object, const api::Class& cls)
    {
        // Hot-path implementation — read the Class's `_nativeHandle`
        // field directly and resolve the canonical name through
        // ReflectionHandleRegistry. No VM round-trip, no bytecode
        // invocation. For closed handles (parameterized types) we use
        // the reified UnifiedType's canonical string; for open handles
        // we use the ClassDefinition's raw name. Both paths produce the
        // same string the language-side Class.getName() would.
        auto instance = asObjectInstance(cls.asValue(), "ScriptAPI::isInstanceOf");
        value::Value handleVal = instance->getFieldValue("_nativeHandle");
        if (!std::holds_alternative<int64_t>(handleVal))
        {
            throw errors::ObjectException(
                "Class._nativeHandle is missing or not an int",
                "Class", "ScriptAPI::isInstanceOf");
        }
        int64_t handle = std::get<int64_t>(handleVal);

        auto& registry = reflection::ReflectionHandleRegistry::instance();
        std::string name;
        auto reified = registry.getReifiedType(handle);
        if (reified)
        {
            name = reified->toCanonicalString();
        }
        else
        {
            auto classDef = registry.getClass(handle);
            if (!classDef)
            {
                throw errors::RuntimeException(
                    "ScriptAPI::isInstanceOf: invalid class handle");
            }
            name = classDef->getName();
        }
        return isInstanceOf(object, name);
    }
}
