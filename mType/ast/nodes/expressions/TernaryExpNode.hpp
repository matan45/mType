#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    class TernaryExpNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> trueExpression;
        std::unique_ptr<ASTNode> falseExpression;

    public:
        explicit TernaryExpNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> trueExpr, std::unique_ptr<ASTNode> falseExpr,
                       const SourceLocation& loc = SourceLocation());
        
        ASTNode* getCondition() const;
        ASTNode* getTrueExpression() const;
        ASTNode* getFalseExpression() const;

        void setCondition(std::unique_ptr<ASTNode> cond);
        void setTrueExpression(std::unique_ptr<ASTNode> trueExpr);
        void setFalseExpression(std::unique_ptr<ASTNode> falseExpr);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
