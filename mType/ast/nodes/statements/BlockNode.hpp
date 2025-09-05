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
        explicit BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, const SourceLocation& loc = SourceLocation());

        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const;
        
        void addStatement(std::unique_ptr<ASTNode> statement);

        void setStatements(std::vector<std::unique_ptr<ASTNode>> stmts);

        size_t getStatementCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
