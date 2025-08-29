#pragma once
#include "IManager.hpp"
#include "../../runtimeTypes/global/NamespaceDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stack>

namespace environment::manager
{
    using namespace runtimeTypes::global;

    class NamespaceManager : public IManager
    {
    private:
        std::shared_ptr<NamespaceDefinition> globalNamespace;
        std::shared_ptr<NamespaceDefinition> currentNamespace;
        std::stack<std::shared_ptr<NamespaceDefinition>> namespaceStack;
        std::unordered_map<std::string, std::shared_ptr<NamespaceDefinition>> allNamespaces;
        std::vector<std::vector<std::string>> usingDirectives;
        std::stack<std::vector<std::vector<std::string>>> usingDirectivesStack;

    public:
        explicit NamespaceManager();
        ~NamespaceManager() override = default;

        void enterScope(const std::string& scopeName = "") override;
        void exitScope() override;
        std::string getCurrentScopeName() const override;
        std::vector<std::string> getScopeHierarchy() const override;
        size_t getScopeDepth() const override;
        
        std::string getComponentName() const override;
        void initialize() override;
        void cleanup() override;
        
        void createNamespace(const std::vector<std::string>& namespacePath);
        void enterNamespace(const std::vector<std::string>& namespacePath);
        void exitNamespace();
        
        std::shared_ptr<NamespaceDefinition> findNamespace(const std::vector<std::string>& namespacePath) const;
        std::shared_ptr<NamespaceDefinition> getCurrentNamespace() const;
        std::shared_ptr<NamespaceDefinition> getGlobalNamespace() const;
        
        std::vector<std::string> getCurrentNamespacePath() const;
        bool isInNamespace(const std::vector<std::string>& namespacePath) const;
        
        void addUsingDirective(const std::vector<std::string>& namespacePath);
        void removeUsingDirective(const std::vector<std::string>& namespacePath);
        const std::vector<std::vector<std::string>>& getUsingDirectives() const;
        
        std::vector<std::string> resolveQualifiedName(const std::string& name) const;
        std::vector<std::vector<std::string>> getAllNamespaces() const;
        
        bool hasNamespace(const std::vector<std::string>& namespacePath) const;
        std::vector<std::vector<std::string>> getNestedNamespaces(const std::vector<std::string>& parentPath) const;
        
        void rebuildHierarchy();
        
    private:
        std::shared_ptr<NamespaceDefinition> findOrCreateNamespace(const std::vector<std::string>& namespacePath);
        void buildNamespaceHierarchy();
    };
}
