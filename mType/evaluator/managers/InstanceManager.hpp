#pragma once
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../environment/Environment.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <vector>
#include <string>

namespace evaluator::managers
{
    using namespace runtimeTypes::klass;
    using namespace environment;
    using namespace value;
    
    /**
     * @brief Manages object instance creation and method calls
     * Separated from ObjectEvaluator following Single Responsibility Principle
     */
    class InstanceManager
    {
    private:
        std::shared_ptr<ObjectInstance> currentInstance;
        
    public:
        explicit InstanceManager();
        ~InstanceManager() = default;
        
        // Instance creation and management
        std::shared_ptr<ObjectInstance> createInstance(
            const std::string& className,
            const std::vector<Value>& constructorArgs,
            std::shared_ptr<Environment> environment);
        
        void setCurrentInstance(std::shared_ptr<ObjectInstance> instance);
        std::shared_ptr<ObjectInstance> getCurrentInstance() const;
        void clearCurrentInstance();
        
        
        Value accessMember(std::shared_ptr<ObjectInstance> object,
                          const std::string& memberName,
                          const SourceLocation& location = SourceLocation()) const;
        void assignMember(std::shared_ptr<ObjectInstance> object,
                         const std::string& memberName,
                         const Value& value,
                         const SourceLocation& location = SourceLocation());

        Value accessStaticMember(const std::string& className,
                                const std::string& memberName,
                                std::shared_ptr<Environment> environment) const;
        void assignStaticMember(const std::string& className,
                               const std::string& memberName,
                               const Value& value,
                               std::shared_ptr<Environment> environment);

        // Copy prevention
        InstanceManager(const InstanceManager&) = delete;
        InstanceManager& operator=(const InstanceManager&) = delete;
    };
}