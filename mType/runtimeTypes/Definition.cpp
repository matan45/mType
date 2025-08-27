#include "Definition.hpp"

namespace runtimeTypes
{
    void Definition::setNamespaceContext(const std::vector<std::string>& context)
    {
        namespaceContext = context;
    }

    void Definition::addNamespaceLevel(const std::string& ns)
    {
        namespaceContext.push_back(ns);
    }

    bool Definition::isInNamespace(const std::vector<std::string>& ns) const
    {
        if (ns.size() > namespaceContext.size()) return false;
        for (size_t i = 0; i < ns.size(); ++i)
        {
            if (namespaceContext[i] != ns[i]) return false;
        }
        return true;
    }

    std::string Definition::getFullyQualifiedName() const
    {
        std::string fqn;
        for (const auto& ns : namespaceContext)
        {
            fqn += ns + "::";
        }
        fqn += name;
        return fqn;
    }
}
