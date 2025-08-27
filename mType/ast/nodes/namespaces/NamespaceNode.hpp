#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::namespaces
{
    class NamespaceNode : public ASTNode
    {
    private:
        std::string namespaceName;
        std::vector<std::unique_ptr<ASTNode>> declarations;

    public:
        explicit NamespaceNode(const std::string& name, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), namespaceName(name) {}

        explicit NamespaceNode(const std::string& name, std::vector<std::unique_ptr<ASTNode>> decls,
                      const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), namespaceName(name), declarations(std::move(decls)) {}

        const std::string& getNamespaceName() const { return namespaceName; }
        const std::vector<std::unique_ptr<ASTNode>>& getDeclarations() const { return declarations; }

        void setNamespaceName(const std::string& name) { namespaceName = name; }
        void addDeclaration(std::unique_ptr<ASTNode> declaration) {
            declarations.push_back(std::move(declaration));
        }

        void setDeclarations(std::vector<std::unique_ptr<ASTNode>> decls) {
            declarations = std::move(decls);
        }

        size_t getDeclarationCount() const { return declarations.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitNamespaceNode(this);
        }
    };
}
