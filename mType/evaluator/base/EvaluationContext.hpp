#pragma once
#include "../../environment/Environment.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <stack>
#include <unordered_map>
#include <string>

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
        bool isInStaticMethod;

        // Current method execution context for generic type resolution
        std::shared_ptr<MethodDefinition> currentMethod;

        // Current constructor execution context for super() calls
        std::shared_ptr<ClassDefinition> currentConstructorClass;

        // Generic type bindings from the current object instance (e.g., T -> String)
        std::unordered_map<std::string, std::string> currentGenericTypeBindings;

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
        bool shouldReturn() const;
        void setReturned(bool returned);
        Value getReturnValue();
        void pushReturnValue(const Value& value);

        // Utility methods to reduce code duplication
        bool isInFunction() const;
        bool isEvaluatingImport() const;

        // Static method context management
        void setInStaticMethod(bool inStatic);
        bool isInStaticMethodContext() const;

        // Current method context management for generic type resolution
        void setCurrentMethod(std::shared_ptr<MethodDefinition> method);
        std::shared_ptr<MethodDefinition> getCurrentMethod() const;
        void clearCurrentMethod();

        // Current constructor context management for super() calls
        void setCurrentConstructorClass(std::shared_ptr<ClassDefinition> classDef);
        std::shared_ptr<ClassDefinition> getCurrentConstructorClass() const;
        void clearCurrentConstructorClass();

        // Generic type binding management
        void setGenericTypeBindings(const std::unordered_map<std::string, std::string>& bindings);
        const std::unordered_map<std::string, std::string>& getGenericTypeBindings() const;
        void clearGenericTypeBindings();
        std::string resolveGenericType(const std::string& typeName) const;
        
        // Copy prevention (context should be shared via shared_ptr)
        EvaluationContext(const EvaluationContext&) = delete;
        EvaluationContext& operator=(const EvaluationContext&) = delete;
    };
}
