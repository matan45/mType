#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class ForNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> initialization;
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> update;
        std::unique_ptr<ASTNode> body;

    public:
        explicit ForNode(std::unique_ptr<ASTNode> init, std::unique_ptr<ASTNode> cond, 
                std::unique_ptr<ASTNode> upd, std::unique_ptr<ASTNode> bodyStmt,
                const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), initialization(std::move(init)), condition(std::move(cond)),
              update(std::move(upd)), body(std::move(bodyStmt)) {}

        std::unique_ptr<ASTNode> getInitialization() const { return initialization.get(); }
        std::unique_ptr<ASTNode> getCondition() const { return condition.get(); }
        std::unique_ptr<ASTNode> getUpdate() const { return update.get(); }
        std::unique_ptr<ASTNode> getBody() const { return body.get(); }

        void setInitialization(std::unique_ptr<ASTNode> init) { initialization = std::move(init); }
        void setCondition(std::unique_ptr<ASTNode> cond) { condition = std::move(cond); }
        void setUpdate(std::unique_ptr<ASTNode> upd) { update = std::move(upd); }
        void setBody(std::unique_ptr<ASTNode> bodyStmt) { body = std::move(bodyStmt); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitForNode(this);
        }
    };
}
