#include "NamespaceManager.hpp"
#include "../util/NamespaceUtils.hpp"
#include <algorithm>

namespace environment::manager
{
    NamespaceManager::NamespaceManager()
    {
        globalNamespace = std::make_shared<NamespaceDefinition>("global");
        currentNamespace = globalNamespace;
        allNamespaces[""] = globalNamespace;
    }

    void NamespaceManager::enterScope(const std::string& scopeName)
    {
        if (!scopeName.empty())
        {
            std::vector<std::string> namespacePath = getCurrentNamespacePath();
            namespacePath.push_back(scopeName);
            enterNamespace(namespacePath);
        }
    }

    void NamespaceManager::exitScope()
    {
        exitNamespace();
    }

    std::string NamespaceManager::getCurrentScopeName() const
    {
        auto path = getCurrentNamespacePath();
        return path.empty() ? "global" : path.back();
    }

    std::vector<std::string> NamespaceManager::getScopeHierarchy() const
    {
        return getCurrentNamespacePath();
    }

    size_t NamespaceManager::getScopeDepth() const
    {
        return getCurrentNamespacePath().size();
    }

    std::string NamespaceManager::getComponentName() const
    {
        return "NamespaceManager";
    }

    void NamespaceManager::initialize()
    {
        globalNamespace = std::make_shared<NamespaceDefinition>("global");
        currentNamespace = globalNamespace;
        allNamespaces.clear();
        allNamespaces[""] = globalNamespace;
        usingDirectives.clear();
        
        while (!namespaceStack.empty())
        {
            namespaceStack.pop();
        }
        
        // Build the initial hierarchy (just global for now)
        buildNamespaceHierarchy();
    }

    void NamespaceManager::cleanup()
    {
        while (!namespaceStack.empty())
        {
            namespaceStack.pop();
        }
        
        currentNamespace = globalNamespace;
        usingDirectives.clear();
    }

    void NamespaceManager::createNamespace(const std::vector<std::string>& namespacePath)
    {
        findOrCreateNamespace(namespacePath);
        // Rebuild hierarchy after creating new namespaces to ensure proper parent-child relationships
        buildNamespaceHierarchy();
    }

    void NamespaceManager::enterNamespace(const std::vector<std::string>& namespacePath)
    {
        auto ns = findOrCreateNamespace(namespacePath);
        if (ns)
        {
            namespaceStack.push(currentNamespace);
            currentNamespace = ns;
        }
    }

    void NamespaceManager::exitNamespace()
    {
        if (!namespaceStack.empty())
        {
            currentNamespace = namespaceStack.top();
            namespaceStack.pop();
        }
        else
        {
            currentNamespace = globalNamespace;
        }
    }

    std::shared_ptr<NamespaceDefinition> NamespaceManager::findNamespace(const std::vector<std::string>& namespacePath) const
    {
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        auto it = allNamespaces.find(nsPath);
        return (it != allNamespaces.end()) ? it->second : nullptr;
    }

    std::shared_ptr<NamespaceDefinition> NamespaceManager::getCurrentNamespace() const
    {
        return currentNamespace;
    }

    std::shared_ptr<NamespaceDefinition> NamespaceManager::getGlobalNamespace() const
    {
        return globalNamespace;
    }

    std::vector<std::string> NamespaceManager::getCurrentNamespacePath() const
    {
        if (!currentNamespace) return {};
        
        std::vector<std::string> path;
        if (currentNamespace != globalNamespace)
        {
            std::string fullName = currentNamespace->getFullyQualifiedName();
            if (!fullName.empty() && fullName != "global")
            {
                path = utils::NamespaceUtils::pathToVector(fullName);
            }
        }
        return path;
    }

    bool NamespaceManager::isInNamespace(const std::vector<std::string>& namespacePath) const
    {
        auto currentPath = getCurrentNamespacePath();
        
        if (namespacePath.size() > currentPath.size()) return false;
        
        for (size_t i = 0; i < namespacePath.size(); ++i)
        {
            if (namespacePath[i] != currentPath[i]) return false;
        }
        
        return true;
    }

    void NamespaceManager::addUsingDirective(const std::vector<std::string>& namespacePath)
    {
        auto it = std::find(usingDirectives.begin(), usingDirectives.end(), namespacePath);
        if (it == usingDirectives.end())
        {
            usingDirectives.push_back(namespacePath);
        }
    }

    void NamespaceManager::removeUsingDirective(const std::vector<std::string>& namespacePath)
    {
        auto it = std::find(usingDirectives.begin(), usingDirectives.end(), namespacePath);
        if (it != usingDirectives.end())
        {
            usingDirectives.erase(it);
        }
    }

    const std::vector<std::vector<std::string>>& NamespaceManager::getUsingDirectives() const
    {
        return usingDirectives;
    }

    std::vector<std::string> NamespaceManager::resolveQualifiedName(const std::string& name) const
    {
        auto currentPath = getCurrentNamespacePath();
        currentPath.push_back(name);
        return currentPath;
    }

    std::vector<std::vector<std::string>> NamespaceManager::getAllNamespaces() const
    {
        std::vector<std::vector<std::string>> namespaces;
        
        for (const auto& [pathStr, ns] : allNamespaces)
        {
            if (!pathStr.empty())
            {
                namespaces.push_back(utils::NamespaceUtils::pathToVector(pathStr));
            }
            else
            {
                namespaces.push_back(std::vector<std::string>{});
            }
        }
        
        return namespaces;
    }

    bool NamespaceManager::hasNamespace(const std::vector<std::string>& namespacePath) const
    {
        return findNamespace(namespacePath) != nullptr;
    }

    std::vector<std::vector<std::string>> NamespaceManager::getNestedNamespaces(const std::vector<std::string>& parentPath) const
    {
        std::vector<std::vector<std::string>> nested;
        std::string parentPathStr = utils::NamespaceUtils::vectorToPath(parentPath);
        
        for (const auto& [pathStr, ns] : allNamespaces)
        {
            if (pathStr.length() > parentPathStr.length())
            {
                if (parentPathStr.empty() || pathStr.substr(0, parentPathStr.length()) == parentPathStr)
                {
                    std::string suffix = parentPathStr.empty() ? pathStr : pathStr.substr(parentPathStr.length());
                    if (!suffix.empty() && suffix.substr(0, 2) == "::")
                    {
                        suffix = suffix.substr(2);
                    }
                    
                    auto suffixParts = utils::NamespaceUtils::pathToVector(suffix);
                    if (suffixParts.size() == 1)
                    {
                        auto fullPath = parentPath;
                        fullPath.push_back(suffixParts[0]);
                        nested.push_back(fullPath);
                    }
                }
            }
        }
        
        return nested;
    }

    std::shared_ptr<NamespaceDefinition> NamespaceManager::findOrCreateNamespace(const std::vector<std::string>& namespacePath)
    {
        if (namespacePath.empty()) return globalNamespace;
        
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        auto it = allNamespaces.find(nsPath);
        if (it != allNamespaces.end())
        {
            return it->second;
        }
        
        auto ns = std::make_shared<NamespaceDefinition>(namespacePath.back());
        ns->setNamespaceContext(std::vector<std::string>(namespacePath.begin(), namespacePath.end() - 1));
        allNamespaces[nsPath] = ns;
        
        return ns;
    }

    void NamespaceManager::buildNamespaceHierarchy()
    {
        // Clear any existing hierarchy
        for (auto& [pathStr, ns] : allNamespaces)
        {
            if (ns && ns != globalNamespace)
            {
                // Reset namespace context based on its path
                auto pathParts = utils::NamespaceUtils::pathToVector(pathStr);
                if (pathParts.size() > 1)
                {
                    std::vector<std::string> parentPath(pathParts.begin(), pathParts.end() - 1);
                    ns->setNamespaceContext(parentPath);
                }
                else
                {
                    ns->setNamespaceContext(std::vector<std::string>{}); // Direct child of global
                }
            }
        }
        
        // Build parent-child relationships
        for (auto& [pathStr, childNs] : allNamespaces)
        {
            if (!childNs || childNs == globalNamespace || pathStr.empty()) continue;
            
            auto pathParts = utils::NamespaceUtils::pathToVector(pathStr);
            if (pathParts.size() > 1)
            {
                // Find parent namespace
                std::vector<std::string> parentPath(pathParts.begin(), pathParts.end() - 1);
                std::string parentPathStr = utils::NamespaceUtils::vectorToPath(parentPath);
                
                auto parentIt = allNamespaces.find(parentPathStr);
                if (parentIt != allNamespaces.end() && parentIt->second)
                {
                    // Establish parent-child relationship
                    // Note: This would require adding parent/children tracking to NamespaceDefinition
                    // For now, we ensure the namespace context is properly set
                    childNs->setNamespaceContext(parentPath);
                }
                else
                {
                    // Parent doesn't exist, create it
                    createNamespace(parentPath);
                    // Recursively build hierarchy for the newly created parent
                    auto newParent = findNamespace(parentPath);
                    if (newParent)
                    {
                        childNs->setNamespaceContext(parentPath);
                    }
                }
            }
            else
            {
                // Direct child of global namespace
                childNs->setNamespaceContext(std::vector<std::string>{});
            }
        }
        
        // Sort namespaces by depth for proper initialization order
        std::vector<std::pair<std::string, std::shared_ptr<NamespaceDefinition>>> sortedNamespaces;
        for (const auto& [pathStr, ns] : allNamespaces)
        {
            if (ns && ns != globalNamespace)
            {
                sortedNamespaces.emplace_back(pathStr, ns);
            }
        }
        
        // Sort by namespace depth (shorter paths first)
        std::sort(sortedNamespaces.begin(), sortedNamespaces.end(),
            [](const auto& a, const auto& b) {
                auto aDepth = std::count(a.first.begin(), a.first.end(), ':') / 2;
                auto bDepth = std::count(b.first.begin(), b.first.end(), ':') / 2;
                return aDepth < bDepth;
            });
        
        // Rebuild allNamespaces in proper order
        std::unordered_map<std::string, std::shared_ptr<NamespaceDefinition>> rebuiltNamespaces;
        rebuiltNamespaces[""] = globalNamespace; // Global namespace first
        
        for (const auto& [pathStr, ns] : sortedNamespaces)
        {
            rebuiltNamespaces[pathStr] = ns;
        }
        
        allNamespaces = std::move(rebuiltNamespaces);
    }

    void NamespaceManager::rebuildHierarchy()
    {
        buildNamespaceHierarchy();
    }
}
