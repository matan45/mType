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
        if (!environment) {
            throw TypeException("Environment is null during object creation");
        }
        
        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' is not defined");
        }
        
        auto instance = std::make_shared<ObjectInstance>(classDef);
        
        // Initialize fields with default values
        for (const auto& fieldPair : classDef->getInstanceFields()) {
            auto field = fieldPair.second;
            instance->setField(field->getName(), field->getValue());
        }
        
        // Call appropriate constructor if arguments provided
        if (!constructorArgs.empty() || !classDef->getConstructors().empty()) {
            auto constructor = classDef->findConstructor(constructorArgs.size());
            if (!constructor) {
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
                                       const std::string& memberName) const
    {
        if (!object) {
            throw TypeException("Cannot access member '" + memberName + "' on null object");
        }

        // Check if it's a field
        if (object->getField(memberName)) {
            return object->getFieldValue(memberName);
        }

        throw UndefinedException("Member '" + memberName + "' not found in class '" +
                                object->getTypeName() + "'");
    }
    
    void InstanceManager::assignMember(std::shared_ptr<ObjectInstance> object, 
                                      const std::string& memberName, 
                                      const Value& value)
    {
        if (!object) {
            throw TypeException("Cannot assign to member '" + memberName + "' on null object");
        }
        
        if (object->getField(memberName)) {
            object->setField(memberName, value);
            return;
        }
        
        throw UndefinedException("Field '" + memberName + "' not found in class '" + 
                                object->getTypeName() + "'");
    }
    
    Value InstanceManager::callMethod(std::shared_ptr<ObjectInstance> object, 
                                     const std::string& methodName,
                                     const std::vector<Value>& args,
                                     std::shared_ptr<Environment> environment)
    {
        if (!object) {
            throw TypeException("Cannot call method '" + methodName + "' on null object");
        }
        
        auto classDef = object->getClassDefinition();
        if (!classDef) {
            throw TypeException("Object has no class definition");
        }
        
        auto method = classDef->findMethod(methodName, args.size());
        if (!method) {
            throw UndefinedException("Method '" + methodName + "' with " + 
                                   std::to_string(args.size()) + 
                                   " arguments not found in class '" + 
                                   object->getTypeName() + "'");
        }
        
        // Method execution would be handled by the calling evaluator
        // This manager only validates and finds the method
        throw TypeException("Method execution should be handled by calling evaluator");
    }
    
    Value InstanceManager::accessStaticMember(const std::string& className, 
                                             const std::string& memberName,
                                             std::shared_ptr<Environment> environment) const
    {
        if (!environment) {
            throw TypeException("Environment is null during static member access");
        }
        
        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' is not defined");
        }
        
        auto field = classDef->getField(memberName);
        if (!field || !field->isStatic()) {
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
        if (!environment) {
            throw TypeException("Environment is null during static member assignment");
        }
        
        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' is not defined");
        }
        
        auto field = classDef->getField(memberName);
        if (!field || !field->isStatic()) {
            throw UndefinedException("Static field '" + memberName + "' not found in class '" + 
                                   className + "'");
        }
        
        if (field->isFinal()) {
            throw TypeException("Cannot modify final static field '" + memberName + "'");
        }
        
        field->setValue(value);
    }
    
    Value InstanceManager::callStaticMethod(const std::string& className, 
                                           const std::string& methodName,
                                           const std::vector<Value>& args,
                                           std::shared_ptr<Environment> environment)
    {
        if (!environment) {
            throw TypeException("Environment is null during static method call");
        }
        
        auto classDef = environment->getClassRegistry()->findItem(className);
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' is not defined");
        }
        
        auto method = classDef->findMethod(methodName, args.size());
        if (!method || !method->isStatic()) {
            throw UndefinedException("Static method '" + methodName + "' with " + 
                                   std::to_string(args.size()) + 
                                   " arguments not found in class '" + className + "'");
        }
        
        // Static method execution would be handled by the calling evaluator
        // This manager only validates and finds the method
        throw TypeException("Static method execution should be handled by calling evaluator");
    }
}