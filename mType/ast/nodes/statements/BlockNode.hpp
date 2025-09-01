#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class BlockNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> statements;

    public:
        explicit BlockNode(const SourceLocation& loc = SourceLocation())
                   : ASTNode(loc){}
        explicit BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), statements(std::move(stmts)) {}

        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const { return statements; }
        
        void addStatement(std::unique_ptr<ASTNode> statement) {
            statements.push_back(std::move(statement));
        }

        void setStatements(std::vector<std::unique_ptr<ASTNode>> stmts) {
            statements = std::move(stmts);
        }

        size_t getStatementCount() const { return statements.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitBlockNode(this);
        }
    };
}
