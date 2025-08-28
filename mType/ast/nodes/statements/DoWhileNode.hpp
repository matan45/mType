#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class DoWhileNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> body;
        std::unique_ptr<ASTNode> condition;

    public:
        explicit DoWhileNode(std::unique_ptr<ASTNode> bodyNode,
                           std::unique_ptr<ASTNode> conditionNode,
                           const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), body(std::move(bodyNode)), condition(std::move(conditionNode)) {}

        ASTNode* getBody() const { return body.get(); }
        ASTNode* getCondition() const { return condition.get(); }

        void setBody(std::unique_ptr<ASTNode> bodyNode) {
            body = std::move(bodyNode);
        }

        void setCondition(std::unique_ptr<ASTNode> conditionNode) {
            condition = std::move(conditionNode);
        }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitDoWhileNode(this);
        }
    };
}