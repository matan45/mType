#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <string>

namespace ast::nodes::namespaces
{
    class UsingNode : public ASTNode
    {
    private:
        std::vector<std::string> namespacePath; // e.g., ["math", "geometry"]

    public:
        explicit UsingNode(const std::vector<std::string>& nsPath, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), namespacePath(nsPath) {}

        const std::vector<std::string>& getNamespacePath() const { return namespacePath; }
        std::string getNamespaceString() const;

        void setNamespacePath(const std::vector<std::string>& nsPath) { namespacePath = nsPath; }
        void addNamespaceLevel(const std::string& level) { namespacePath.push_back(level); }

        bool isEmpty() const { return namespacePath.empty(); }
        size_t getPathDepth() const { return namespacePath.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitUsingNode(this);
        }
    };
}
