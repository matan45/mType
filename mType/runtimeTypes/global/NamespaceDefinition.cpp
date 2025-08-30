#include "NamespaceDefinition.hpp"

namespace runtimeTypes::global
{
    void NamespaceDefinition::merge(const NamespaceDefinition& other)
    {
    }
    
    void NamespaceDefinition::addUsingDirective(const std::vector<std::string>& namespacePath)
    {
        // Avoid duplicates
        for (const auto& existing : usingNamespaces) {
            if (existing == namespacePath) {
                return;
            }
        }
        usingNamespaces.push_back(namespacePath);
    }
    
    const std::vector<std::vector<std::string>>& NamespaceDefinition::getUsingDirectives() const
    {
        return usingNamespaces;
    }
}
