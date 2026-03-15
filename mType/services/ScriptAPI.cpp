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
#include "../vm/bytecode/BytecodeProgram.hpp"
#include <iostream>

namespace services
{
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
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot call method on non-object value", "", __FUNCTION__);
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);

        // Use VM for method invocation
        if (vm)
        {
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
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot access field on non-object value", "", __FUNCTION__);
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);

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
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot set field on non-object value", "", __FUNCTION__);
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);

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
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            return false;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName() == className;
    }

    std::string ScriptAPI::getObjectClassName(const value::Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::TypeConversionException("Value is not an object", "unknown", "object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }
}
