#include "EvaluationContext.hpp"

namespace evaluator::base
{
    EvaluationContext::EvaluationContext(std::shared_ptr<Environment> env)
        : environment(env), currentInstance(nullptr), hasReturned(false),
          cachedEnv(nullptr), envCacheValid(false)
    {
    }
    
    std::shared_ptr<Environment> EvaluationContext::getEnvironment() const
    {
        if (!envCacheValid) {
            cachedEnv = environment;
            envCacheValid = true;
        }
        return cachedEnv;
    }
    
    void EvaluationContext::invalidateEnvironmentCache()
    {
        envCacheValid = false;
        cachedEnv = nullptr;
    }
    
    void EvaluationContext::setCurrentInstance(std::shared_ptr<ObjectInstance> instance)
    {
        currentInstance = instance;
    }
    
    std::shared_ptr<ObjectInstance> EvaluationContext::getCurrentInstance() const
    {
        return currentInstance;
    }
    
    void EvaluationContext::clearCurrentInstance()
    {
        currentInstance = nullptr;
    }
    
    Value EvaluationContext::getReturnValue()
    {
        if (!returnStack.empty()) {
            Value val = returnStack.top();
            returnStack.pop();
            return val;
        }
        return std::monostate{};
    }
    
    void EvaluationContext::pushReturnValue(const Value& value)
    {
        returnStack.push(value);
    }
    
    bool EvaluationContext::isInFunction() const
    {
        return environment && environment->isInFunction();
    }
    
    bool EvaluationContext::isEvaluatingImport() const
    {
        return environment && environment->isEvaluatingImport();
    }
}