#pragma once
#include "../../environment/registry/VariableDefinition.hpp"
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::manager
{
    using namespace runtimeTypes::global;

    enum class ScopeType
    {
        GLOBAL,
        CLASS,
        FUNCTION,
        BLOCK,
        LOOP,
        CONDITIONAL
    };

    class Scope : public std::enable_shared_from_this<Scope>
    {
    private:
        std::string name;
        ScopeType type;
        std::shared_ptr<Scope> parent;
        std::vector<std::shared_ptr<Scope>> children;
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> variables;

    public:
        Scope(const std::string& scopeName, ScopeType scopeType, std::shared_ptr<Scope> parentScope = nullptr);
        ~Scope() = default;

        const std::string& getName() const;
        ScopeType getType() const;
        std::shared_ptr<Scope> getParent() const;
        const std::vector<std::shared_ptr<Scope>>& getChildren() const;
        
        void addChild(std::shared_ptr<Scope> child);
        void removeChild(const std::string& childName);
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        std::shared_ptr<VariableDefinition> findVariableInCurrentScope(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        bool hasVariableInCurrentScope(const std::string& varName) const;
        
        std::vector<std::string> getAllVariableNames() const;
        // Walks the parent chain and returns every variable name visible
        // from this scope, deduplicated. Used by the diagnostic
        // IdentifierEnumerator to build "did you mean" pools.
        std::vector<std::string> getAllVisibleVariableNames() const;
        size_t getVariableCount() const;
        
        std::shared_ptr<Scope> findScope(const std::string& scopeName) const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getDepth() const;
    };
}

