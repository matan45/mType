#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class CaseNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> value;
        std::vector<std::unique_ptr<ASTNode>> statements;

    public:
        explicit CaseNode(std::unique_ptr<ASTNode> caseValue, const SourceLocation& loc = SourceLocation());

        ASTNode* getValue() const;
        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const;

        void setValue(std::unique_ptr<ASTNode> caseValue);
        void addStatement(std::unique_ptr<ASTNode> statement);

        size_t getStatementCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
