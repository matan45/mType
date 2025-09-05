#pragma once

#include <memory>
#include <string>
#include "../../environment/Environment.hpp"
#include "../../environment/manager/Scope.hpp"

namespace evaluator::utils
{
    /**
     * RAII-style scope management utility that automatically handles
     * scope entry/exit with proper exception safety.
     */
    class ScopeGuard
    {
    public:
        /**
         * Enters a new scope immediately upon construction
         */
        ScopeGuard(
            std::shared_ptr<environment::Environment> env,
            const std::string& scopeName,
            environment::manager::ScopeType scopeType = environment::manager::ScopeType::BLOCK
        );

        /**
         * Automatically exits the scope when destroyed
         */
        ~ScopeGuard();

        // Prevent copying to ensure unique ownership
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;

        // Allow moving
        ScopeGuard(ScopeGuard&& other) noexcept;
        ScopeGuard& operator=(ScopeGuard&& other) noexcept;

        /**
         * Manually exit the scope early (optional)
         * Scope will not be exited again in destructor
         */
        void exitEarly();

        /**
         * Check if scope is still active
         */
        bool isActive() const { return scopeEntered; }

    private:
        std::shared_ptr<environment::Environment> environment;
        bool scopeEntered;
    };

    /**
     * Convenience function for executing code within a managed scope
     */
    template<typename Executor>
    auto withScope(
        std::shared_ptr<environment::Environment> env,
        const std::string& scopeName,
        environment::manager::ScopeType scopeType,
        Executor&& executor
    ) -> decltype(executor())
    {
        ScopeGuard guard(env, scopeName, scopeType);
        return executor();
    }
}