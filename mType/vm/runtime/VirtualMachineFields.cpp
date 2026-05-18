#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/FieldNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "utils/InteropExceptionDecorator.hpp"

namespace vm::runtime
{
    value::Value VirtualMachine::getField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                          const std::string& fieldName)
    {
        try
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
        catch (errors::ScriptException& e)
        {
            // MYT-46: attribute the failure to the innermost script call
            // frame, if any. Pure-C++ callers (empty stack) get header-only
            // diagnostics, which is correct.
            utils::decorateFromCallStack(e, callStack, program);
            throw;
        }
    }

    void VirtualMachine::setField(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                  const std::string& fieldName,
                                  const value::Value& value)
    {
        try
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

            // Static finals share FieldDefinition's isInitialized bit; instance
            // finals track state per-instance via the fieldValues map. MYT-189:
            // match ObjectExecutor's SET_FIELD check so repeated construction
            // of instances with final fields doesn't trip the check.
            if (fieldDef->isFinal())
            {
                if (fieldDef->isStatic())
                {
                    if (fieldDef->isInitialized())
                    {
                        throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
                    }
                }
                else
                {
                    size_t idx = instance->getClassDefinition()->getFieldIndex(fieldName);
                    if (idx != SIZE_MAX)
                    {
                        if (!value::isVoid(instance->getFieldByIndex(idx)))
                        {
                            throw errors::RuntimeException("Cannot assign to final field '" + fieldName + "'");
                        }
                    }
                }
            }

            if (fieldDef->isStatic())
            {
                fieldDef->setValue(value);
            }
            else
            {
                instance->setField(fieldName, value);
            }
        }
        catch (errors::ScriptException& e)
        {
            utils::decorateFromCallStack(e, callStack, program);
            throw;
        }
    }

    value::Value VirtualMachine::getStaticField(const std::string& className, const std::string& fieldName)
    {
        try
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
        catch (errors::ScriptException& e)
        {
            utils::decorateFromCallStack(e, callStack, program);
            throw;
        }
    }

    void VirtualMachine::setStaticField(const std::string& className,
                                        const std::string& fieldName,
                                        const value::Value& value)
    {
        try
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
        catch (errors::ScriptException& e)
        {
            utils::decorateFromCallStack(e, callStack, program);
            throw;
        }
    }
}
