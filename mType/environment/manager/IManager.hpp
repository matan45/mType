#pragma once
#include "../IEnvironmentComponent.hpp"
#include <string>
#include <vector>

namespace environment::manager
{
    class IManager : public IEnvironmentComponent
    {
    public:
        virtual ~IManager() = default;
        
        virtual void enterScope(const std::string& scopeName = "") = 0;
        virtual void exitScope() = 0;
        virtual std::string getCurrentScopeName() const = 0;
        virtual std::vector<std::string> getScopeHierarchy() const = 0;
        virtual size_t getScopeDepth() const = 0;
        
        void initialize() override {}
        void cleanup() override {}
    };
}