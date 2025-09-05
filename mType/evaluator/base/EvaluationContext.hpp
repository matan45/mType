#pragma once
#include "../../environment/Environment.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <stack>

namespace evaluator::base
{
    using namespace environment;
    using namespace value;
    using namespace runtimeTypes::klass;
    
    /**
     * @brief Centralized evaluation context following Single Responsibility Principle
     * Manages shared state between different evaluators to reduce coupling
     */
    class EvaluationContext
    {
    private:
        std::shared_ptr<Environment> environment;
        std::shared_ptr<ObjectInstance> currentInstance;
        std::stack<Value> returnStack;
        bool hasReturned;
        
        // Performance optimization: cache frequently accessed values
        mutable std::shared_ptr<Environment> cachedEnv;
        mutable bool envCacheValid;
        
    public:
        explicit EvaluationContext(std::shared_ptr<Environment> env);
        ~EvaluationContext() = default;
        
        // Environment management with caching
        std::shared_ptr<Environment> getEnvironment() const;
        void invalidateEnvironmentCache();
        
        // Object instance management
        void setCurrentInstance(std::shared_ptr<ObjectInstance> instance);
        std::shared_ptr<ObjectInstance> getCurrentInstance() const;
        void clearCurrentInstance();
        
        // Return value management
        bool shouldReturn() const { return hasReturned; }
        void setReturned(bool returned) { hasReturned = returned; }
        Value getReturnValue();
        void pushReturnValue(const Value& value);
        
        // Utility methods to reduce code duplication
        bool isInFunction() const;
        bool isEvaluatingImport() const;
        
        // Copy prevention (context should be shared via shared_ptr)
        EvaluationContext(const EvaluationContext&) = delete;
        EvaluationContext& operator=(const EvaluationContext&) = delete;
    };
}