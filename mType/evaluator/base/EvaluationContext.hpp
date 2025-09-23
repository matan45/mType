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
        bool shouldReturn() const { return hasReturned; }
        void setReturned(bool returned) { hasReturned = returned; }
        Value getReturnValue();
        void pushReturnValue(const Value& value);
        
        // Utility methods to reduce code duplication
        bool isInFunction() const;
        bool isEvaluatingImport() const;

        // Static method context management
        void setInStaticMethod(bool inStatic) { isInStaticMethod = inStatic; }
        bool isInStaticMethodContext() const { return isInStaticMethod; }

        // Current method context management for generic type resolution
        void setCurrentMethod(std::shared_ptr<MethodDefinition> method) { currentMethod = method; }
        std::shared_ptr<MethodDefinition> getCurrentMethod() const { return currentMethod; }
        void clearCurrentMethod() { currentMethod = nullptr; }

        // Generic type binding management
        void setGenericTypeBindings(const std::unordered_map<std::string, std::string>& bindings) {
            currentGenericTypeBindings = bindings;
        }
        const std::unordered_map<std::string, std::string>& getGenericTypeBindings() const {
            return currentGenericTypeBindings;
        }
        void clearGenericTypeBindings() { currentGenericTypeBindings.clear(); }
        std::string resolveGenericType(const std::string& typeName) const {
            auto it = currentGenericTypeBindings.find(typeName);
            return (it != currentGenericTypeBindings.end()) ? it->second : typeName;
        }

        // Copy prevention (context should be shared via shared_ptr)
        EvaluationContext(const EvaluationContext&) = delete;
        EvaluationContext& operator=(const EvaluationContext&) = delete;
    };
}