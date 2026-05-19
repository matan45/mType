#include "ScriptAPI.hpp"
#include <cstddef>
#include "../environment/registry/ClassDefinition.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ValueShim.hpp"
#include "../environment/registry/ConstructorDefinition.hpp"
#include "../environment/registry/FunctionDefinition.hpp"
#include "../environment/registry/VariableDefinition.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtime/EventLoop.hpp"
#include "../value/PromiseValue.hpp"

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
            // MYT-208: accept STACK_OBJECT (raw borrowed) alongside OBJECT.
            // ScriptAPI is interop / not hot-path, so wrapping the raw pointer
            // in an aliasing shared_ptr with a no-op deleter is acceptable —
            // lifetime stays with the owning CallFrame::stackObjects array.
            if (value::isStackObject(v))
            {
                auto* raw = value::asObjectInstanceRaw(v);
                if (!raw)
                {
                    throw errors::ObjectException(
                        "Value is a null object", "", apiName);
                }
                return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
                    raw, [](runtimeTypes::klass::ObjectInstance*) {});
            }
            if (!value::isObject(v))
            {
                throw errors::ObjectException(
                    "Value is not an object", "", apiName);
            }
            auto instance = value::asObject(v);
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
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
        {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc)
            {
                ::environment::NativeContext nativeCtx{ this->environment, this->vm->shared_from_this() };
                return nativeFunc(nativeCtx, args);
            }
        }

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
        if (!value::isLambda(lambda))
        {
            throw errors::ObjectException("Cannot invoke non-lambda value", "", __FUNCTION__);
        }

        auto lambdaPtr = value::asLambda(lambda);

        if (vm)
        {
            return vm->invokeLambda(lambdaPtr, args);
        }

        throw errors::RuntimeException("Cannot invoke lambda: VM not available");
    }

    value::Value ScriptAPI::getStaticField(const std::string& className, const std::string& fieldName)
    {
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
        if (vm)
        {
            vm->setStaticField(className, fieldName, value);
            return;
        }

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

        if (vm)
        {
            return vm->getField(instance, fieldName);
        }

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

        if (vm)
        {
            vm->setField(instance, fieldName, value);
            return;
        }

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
        if (!value::isObject(object))
        {
            return false;
        }
        auto instance = value::asObject(object);
        if (!instance)
        {
            return false;
        }
        auto classDef = instance->getClassDefinition();
        return classDef && classDef->getName() == className;
    }

    std::string ScriptAPI::getObjectClassName(const value::Value& object)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::getObjectClassName");
        return instance->getClassDefinition()->getName();
    }
}
