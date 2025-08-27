#pragma once
#include <string>
#include <vector>

namespace runtimeTypes
{
    class Definition
    {
    private:
        std::string name;
        std::vector<std::string> namespaceContext;

    public:
        explicit Definition(const std::string& n = "") : name(n)
        {
        }

        virtual ~Definition() = default;

        // Set namespace context
        void setNamespaceContext(const std::vector<std::string>& context);

        // Add to existing namespace context
        void addNamespaceLevel(const std::string& ns);
        
        // Check if in namespace
        bool isInNamespace(const std::vector<std::string>& ns) const;

        std::string getFullyQualifiedName() const;
        
    };
}
