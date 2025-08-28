#pragma once
#include "../../runtimeTypes/global/VariableDefinition.hpp"
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
        NAMESPACE,
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
        std::vector<std::string> namespacePath;

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
        size_t getVariableCount() const;
        
        void setNamespacePath(const std::vector<std::string>& path);
        const std::vector<std::string>& getNamespacePath() const;
        
        std::shared_ptr<Scope> findScope(const std::string& scopeName) const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getDepth() const;
    };
}

