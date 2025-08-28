#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class DefaultCaseNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> statements;

    public:
        explicit DefaultCaseNode(const SourceLocation& loc = SourceLocation())
            : ASTNode(loc) {}

        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const { return statements; }
        
        void addStatement(std::unique_ptr<ASTNode> statement) { statements.push_back(std::move(statement)); }

        size_t getStatementCount() const { return statements.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitDefaultCaseNode(this);
        }
    };
}

