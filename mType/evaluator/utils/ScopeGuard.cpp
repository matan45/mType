#include "ScopeGuard.hpp"

namespace evaluator::utils
{
    using namespace environment;
    using namespace environment::manager;

    ScopeGuard::ScopeGuard(
        std::shared_ptr<Environment> env,
        const std::string& scopeName,
        ScopeType scopeType)
        : environment(env), scopeEntered(true)
    {
        if (environment) {
            environment->enterScope(scopeName, scopeType);
        } else {
            scopeEntered = false;
        }
    }

    ScopeGuard::~ScopeGuard()
    {
        if (scopeEntered && environment) {
            environment->exitScope();
        }
    }

    ScopeGuard::ScopeGuard(ScopeGuard&& other) noexcept
        : environment(std::move(other.environment)), scopeEntered(other.scopeEntered)
    {
        other.scopeEntered = false;
    }

    ScopeGuard& ScopeGuard::operator=(ScopeGuard&& other) noexcept
    {
        if (this != &other) {
            // Clean up current scope if active
            if (scopeEntered && environment) {
                environment->exitScope();
            }

            // Move from other
            environment = std::move(other.environment);
            scopeEntered = other.scopeEntered;
            other.scopeEntered = false;
        }
        return *this;
    }

    void ScopeGuard::exitEarly()
    {
        if (scopeEntered && environment) {
            environment->exitScope();
            scopeEntered = false;
        }
    }
}