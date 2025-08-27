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
        explicit DoWhileNode(std::unique_ptr<ASTNode> bodyStmt, std::unique_ptr<ASTNode> cond, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), body(std::move(bodyStmt)), condition(std::move(cond)) {}

        ASTNode* getBody() const { return body.get(); }
        ASTNode* getCondition() const { return condition.get(); }

        void setBody(std::unique_ptr<ASTNode> bodyStmt) { body = std::move(bodyStmt); }
        void setCondition(std::unique_ptr<ASTNode> cond) { condition = std::move(cond); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitDoWhileNode(this);
        }
    };
}
