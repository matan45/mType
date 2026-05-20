#pragma once
#include "../../ASTNode.hpp"
#include <cstddef>
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class BlockNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> statements;
        // Per-scope NEW_STACK release marker. Set by EscapeAnalysisPass's
        // post-pass when this block's subtree transitively contains a
        // NewNode with isStackAllocated() == true. The bytecode compiler
        // gates STACK_SCOPE_ENTER / STACK_SCOPE_LEAVE emission on this bit
        // so blocks without any stack-promoted allocs pay zero overhead.
        bool containsStackAlloc_ = false;

    public:
        explicit BlockNode(const SourceLocation& loc = SourceLocation())
                   : ASTNode(loc){}
        explicit BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, const SourceLocation& loc = SourceLocation());

        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const;

        void addStatement(std::unique_ptr<ASTNode> statement);

        void setStatements(std::vector<std::unique_ptr<ASTNode>> stmts);

        size_t getStatementCount() const;

        bool containsStackAlloc() const { return containsStackAlloc_; }
        void setContainsStackAlloc(bool value) { containsStackAlloc_ = value; }

        Value accept(ASTVisitor<Value>& visitor) override;

        std::unique_ptr<ASTNode> clone() const override;
    };
}
