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
        explicit DefaultCaseNode(const SourceLocation& loc = SourceLocation());

        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const;

        void addStatement(std::unique_ptr<ASTNode> statement);

        size_t getStatementCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}

