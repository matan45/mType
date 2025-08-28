#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class WhileNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;

    public:
        explicit WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> bodyStmt, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), condition(std::move(cond)), body(std::move(bodyStmt)) {}

        ASTNode* getCondition() const { return condition.get(); }
        ASTNode* getBody() const { return body.get(); }

        void setCondition(std::unique_ptr<ASTNode> cond) { condition = std::move(cond); }
        void setBody(std::unique_ptr<ASTNode> bodyStmt) { body = std::move(bodyStmt); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitWhileNode(this);
        }
    };
}
