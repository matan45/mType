#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class SwitchNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> expression;
        std::vector<std::unique_ptr<ASTNode>> cases; // CaseNode and DefaultCaseNode

    public:
        explicit SwitchNode(std::unique_ptr<ASTNode> expr, const SourceLocation& loc = SourceLocation());

        ASTNode* getExpression() const;
        const std::vector<std::unique_ptr<ASTNode>>& getCases() const;

        void setExpression(std::unique_ptr<ASTNode> expr);
        void addCase(std::unique_ptr<ASTNode> caseNode);
        
        size_t getCaseCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
