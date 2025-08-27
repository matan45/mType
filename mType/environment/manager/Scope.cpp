#include "Scope.hpp"
#include <algorithm>

namespace environment::manager
{
    Scope::Scope(const std::string& scopeName, ScopeType scopeType, std::shared_ptr<Scope> parentScope)
        : name(scopeName), type(scopeType), parent(parentScope)
    {
        if (parent)
        {
            namespacePath = parent->getNamespacePath();
            if (type == ScopeType::NAMESPACE)
            {
                namespacePath.push_back(scopeName);
            }
        }
    }

    const std::string& Scope::getName() const
    {
        return name;
    }

    ScopeType Scope::getType() const
    {
        return type;
    }

    std::shared_ptr<Scope> Scope::getParent() const
    {
        return parent;
    }

    const std::vector<std::shared_ptr<Scope>>& Scope::getChildren() const
    {
        return children;
    }

    void Scope::addChild(std::shared_ptr<Scope> child)
    {
        if (child && std::find(children.begin(), children.end(), child) == children.end())
        {
            children.push_back(child);
        }
    }

    void Scope::removeChild(const std::string& childName)
    {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [&childName](const std::shared_ptr<Scope>& child) {
                    return child && child->getName() == childName;
                }),
            children.end());
    }

    void Scope::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        variables[varName] = variable;
        if (variable)
        {
            variable->setNamespaceContext(namespacePath);
        }
    }

    std::shared_ptr<VariableDefinition> Scope::findVariable(const std::string& varName) const
    {
        auto it = variables.find(varName);
        if (it != variables.end())
        {
            return it->second;
        }
        
        if (parent)
        {
            return parent->findVariable(varName);
        }
        
        return nullptr;
    }

    std::shared_ptr<VariableDefinition> Scope::findVariableInCurrentScope(const std::string& varName) const
    {
        auto it = variables.find(varName);
        return (it != variables.end()) ? it->second : nullptr;
    }

    bool Scope::hasVariable(const std::string& varName) const
    {
        return findVariable(varName) != nullptr;
    }

    bool Scope::hasVariableInCurrentScope(const std::string& varName) const
    {
        return variables.find(varName) != variables.end();
    }

    std::vector<std::string> Scope::getAllVariableNames() const
    {
        std::vector<std::string> names;
        for (const auto& [varName, _] : variables)
        {
            names.push_back(varName);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t Scope::getVariableCount() const
    {
        return variables.size();
    }

    void Scope::setNamespacePath(const std::vector<std::string>& path)
    {
        namespacePath = path;
    }

    const std::vector<std::string>& Scope::getNamespacePath() const
    {
        return namespacePath;
    }

    std::shared_ptr<Scope> Scope::findScope(const std::string& scopeName) const
    {
        if (name == scopeName)
        {
            return std::const_pointer_cast<Scope>(shared_from_this());
        }
        
        for (const auto& child : children)
        {
            if (auto found = child->findScope(scopeName))
            {
                return found;
            }
        }
        
        return nullptr;
    }

    std::vector<std::string> Scope::getScopeHierarchy() const
    {
        std::vector<std::string> hierarchy;
        
        std::shared_ptr<const Scope> current = shared_from_this();
        while (current)
        {
            hierarchy.insert(hierarchy.begin(), current->getName());
            current = current->getParent();
        }
        
        return hierarchy;
    }

    size_t Scope::getDepth() const
    {
        size_t depth = 0;
        std::shared_ptr<const Scope> current = parent;
        while (current)
        {
            ++depth;
            current = current->getParent();
        }
        return depth;
    }
}
