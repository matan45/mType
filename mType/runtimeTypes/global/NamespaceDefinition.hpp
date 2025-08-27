#pragma once
#include <unordered_map>
#include <memory>
#include "../Definition.hpp"
#include "VariableDefinition.hpp"
#include "FunctionDefinition.hpp"
#include "../klass/ClassDefinition.hpp"
#include "../klass/ObjectInstance.hpp"

namespace runtimeTypes::global
{
    using namespace klass;

    class NamespaceDefinition : public Definition
    {
    private:
        // Namespace members
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> variables;
        std::unordered_map<std::string, std::shared_ptr<FunctionDefinition>> functions;
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classes;
        std::unordered_map<std::string, std::shared_ptr<ObjectInstance>> objects;
        std::unordered_map<std::string, std::shared_ptr<NamespaceDefinition>> nestedNamespaces;

        // Using directives within this namespace
        std::vector<std::vector<std::string>> usingNamespaces;

    public:
        explicit NamespaceDefinition(const std::string& n) : Definition(n)
        {
        }

        // Find member by name (searches nested namespaces too)
        template <typename T>
        std::shared_ptr<T> find(const std::string& qualifiedName) const;

        // Merge another namespace (for extending namespaces)
        void merge(const NamespaceDefinition& other);
    };
}
