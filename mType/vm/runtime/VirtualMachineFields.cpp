#include "VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/FieldNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"

namespace vm::runtime
{
    value::Value VirtualMachine::getField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                          const std::string& fieldName)
    {
        if (!instance)
        {
            throw errors::NullPointerException("Cannot access field '" + fieldName + "' on null object");
        }

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        return fieldDef->getValue();
    }

    void VirtualMachine::setField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                  const std::string& fieldName,
                                  const value::Value& value)
    {
        if (!instance)
        {
            throw errors::NullPointerException("Cannot set field '" + fieldName + "' on null object");
        }

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, instance->getClassDefinition()->getName());
        }

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
        }

        fieldDef->setValue(value);
    }

    value::Value VirtualMachine::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef || !fieldDef->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        return fieldDef->getValue();
    }

    void VirtualMachine::setStaticField(const std::string& className,
                                        const std::string& fieldName,
                                        const value::Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto fieldDef = classDef->getField(fieldName);
        if (!fieldDef || !fieldDef->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::RuntimeException("Cannot assign to final static field '" + fieldName + "'");
        }

        fieldDef->setValue(value);
    }
}
