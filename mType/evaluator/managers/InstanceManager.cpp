#include "InstanceManager.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../environment/Environment.hpp"

namespace evaluator::managers
{
    using namespace errors;
    using namespace environment;

    InstanceManager::InstanceManager()
        : currentInstance(nullptr)
    {
    }

    std::shared_ptr<ObjectInstance> InstanceManager::createInstance(
        const std::string& className,
        const std::vector<Value>& constructorArgs,
        std::shared_ptr<Environment> environment)
    {
        if (!environment)
        {
            throw TypeException("Environment is null during object creation");
        }

        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' is not defined");
        }

        auto instance = std::make_shared<ObjectInstance>(classDef);

        // Initialize fields with default values
        for (const auto& fieldPair : classDef->getInstanceFields())
        {
            auto field = fieldPair.second;
            instance->setField(field->getName(), field->getValue());
        }

        // Call appropriate constructor if arguments provided
        if (!constructorArgs.empty() || !classDef->getConstructors().empty())
        {
            auto constructor = classDef->findConstructor(constructorArgs.size());
            if (!constructor)
            {
                throw ArgumentException("No matching constructor found for class '" +
                    className + "' with " +
                    std::to_string(constructorArgs.size()) + " arguments");
            }

            // Constructor execution would be handled by the calling evaluator
            // This manager only creates the instance structure
        }

        return instance;
    }

    void InstanceManager::setCurrentInstance(std::shared_ptr<ObjectInstance> instance)
    {
        currentInstance = instance;
    }

    std::shared_ptr<ObjectInstance> InstanceManager::getCurrentInstance() const
    {
        return currentInstance;
    }

    void InstanceManager::clearCurrentInstance()
    {
        currentInstance = nullptr;
    }

    Value InstanceManager::accessMember(std::shared_ptr<ObjectInstance> object,
                                        const std::string& memberName,
                                        const errors::SourceLocation& location) const
    {
        if (!object)
        {
            throw TypeException("Cannot access member '" + memberName + "' on null object", location);
        }

        // Check if it's a field
        if (object->getField(memberName))
        {
            return object->getFieldValue(memberName);
        }

        throw UndefinedException("Member '" + memberName + "' not found in class '" +
                                 object->getTypeName() + "'", location);
    }

    void InstanceManager::assignMember(std::shared_ptr<ObjectInstance> object,
                                       const std::string& memberName,
                                       const Value& value,
                                       const SourceLocation& location)
    {
        if (!object)
        {
            throw TypeException("Cannot assign to member '" + memberName + "' on null object", location);
        }

        if (object->getField(memberName))
        {
            object->setField(memberName, value);
            return;
        }

        throw UndefinedException("Field '" + memberName + "' not found in class '" +
                                 object->getTypeName() + "'", location);
    }

    Value InstanceManager::accessStaticMember(const std::string& className,
                                              const std::string& memberName,
                                              std::shared_ptr<Environment> environment) const
    {
        if (!environment)
        {
            throw TypeException("Environment is null during static member access");
        }

        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' is not defined");
        }

        auto field = classDef->getField(memberName);
        if (!field || !field->isStatic())
        {
            throw UndefinedException("Static field '" + memberName + "' not found in class '" +
                className + "'");
        }

        return field->getValue();
    }

    void InstanceManager::assignStaticMember(const std::string& className,
                                             const std::string& memberName,
                                             const Value& value,
                                             std::shared_ptr<Environment> environment)
    {
        if (!environment)
        {
            throw TypeException("Environment is null during static member assignment");
        }

        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' is not defined");
        }

        auto field = classDef->getField(memberName);
        if (!field || !field->isStatic())
        {
            throw UndefinedException("Static field '" + memberName + "' not found in class '" +
                className + "'");
        }

        if (field->isFinal())
        {
            throw TypeException("Cannot modify final static field '" + memberName + "'");
        }

        field->setValue(value);
    }
}
