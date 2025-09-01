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
        explicit SwitchNode(std::unique_ptr<ASTNode> expr, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), expression(std::move(expr)) {}

        ASTNode* getExpression() const { return expression.get(); }
        const std::vector<std::unique_ptr<ASTNode>>& getCases() const { return cases; }

        void setExpression(std::unique_ptr<ASTNode> expr) { expression = std::move(expr); }
        void addCase(std::unique_ptr<ASTNode> caseNode) { cases.push_back(std::move(caseNode)); }
        
        size_t getCaseCount() const { return cases.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitSwitchNode(this);
        }
    };
}
