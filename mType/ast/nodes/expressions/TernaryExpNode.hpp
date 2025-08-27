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
                       const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), condition(std::move(cond)), trueExpression(std::move(trueExpr)), falseExpression(std::move(falseExpr)) {}

        ASTNode* getCondition() const { return condition.get(); }
        ASTNode* getTrueExpression() const { return trueExpression.get(); }
        ASTNode* getFalseExpression() const { return falseExpression.get(); }

        void setCondition(std::unique_ptr<ASTNode> cond) { condition = std::move(cond); }
        void setTrueExpression(std::unique_ptr<ASTNode> trueExpr) { trueExpression = std::move(trueExpr); }
        void setFalseExpression(std::unique_ptr<ASTNode> falseExpr) { falseExpression = std::move(falseExpr); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitTernaryOpNode(this);
        }
    };
}
