#include "EvaluationContext.hpp"

namespace evaluator::base
{
    EvaluationContext::EvaluationContext(std::shared_ptr<Environment> env)
        : environment(env), currentInstance(nullptr), hasReturned(false),
          isInStaticMethod(false), inSuperInitializerContext(false), currentMethod(nullptr),
          currentConstructorClass(nullptr), cachedEnv(nullptr), envCacheValid(false)
    {
    }

    std::shared_ptr<Environment> EvaluationContext::getEnvironment() const
    {
        if (!envCacheValid)
        {
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

    bool EvaluationContext::shouldReturn() const
    { return hasReturned; }

    void EvaluationContext::setReturned(bool returned)
    { hasReturned = returned; }

    Value EvaluationContext::getReturnValue()
    {
        if (!returnStack.empty())
        {
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

    void EvaluationContext::setInStaticMethod(bool inStatic)
    { isInStaticMethod = inStatic; }

    bool EvaluationContext::isInStaticMethodContext() const
    { return isInStaticMethod; }

    void EvaluationContext::setCurrentMethod(std::shared_ptr<MethodDefinition> method)
    { currentMethod = method; }

    std::shared_ptr<MethodDefinition> EvaluationContext::getCurrentMethod() const
    { return currentMethod; }

    void EvaluationContext::clearCurrentMethod()
    { currentMethod = nullptr; }

    void EvaluationContext::setCurrentConstructorClass(std::shared_ptr<ClassDefinition> classDef)
    { currentConstructorClass = classDef; }

    std::shared_ptr<ClassDefinition> EvaluationContext::getCurrentConstructorClass() const
    { return currentConstructorClass; }

    void EvaluationContext::clearCurrentConstructorClass()
    { currentConstructorClass = nullptr; }

    void EvaluationContext::setGenericTypeBindings(const std::unordered_map<std::string, std::string>& bindings)
    {
        currentGenericTypeBindings = bindings;
    }

    const std::unordered_map<std::string, std::string>& EvaluationContext::getGenericTypeBindings() const
    {
        return currentGenericTypeBindings;
    }

    void EvaluationContext::clearGenericTypeBindings()
    { currentGenericTypeBindings.clear(); }

    std::string EvaluationContext::resolveGenericType(const std::string& typeName) const
    {
        auto it = currentGenericTypeBindings.find(typeName);
        return (it != currentGenericTypeBindings.end()) ? it->second : typeName;
    }

    void EvaluationContext::setInSuperInitializerContext(bool inContext)
    { inSuperInitializerContext = inContext; }

    bool EvaluationContext::isInSuperInitializerContext() const
    { return inSuperInitializerContext; }
}
