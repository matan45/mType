#pragma once
#include "../../ASTNode.hpp"
#include "CatchNode.hpp"
#include <memory>
#include <vector>

namespace ast::nodes::statements
{
    /**
     * @brief AST node representing a try-catch-finally statement
     *
     * Syntax: try { ... } catch (Type var) { ... } finally { ... }
     * Handles exceptions with optional catch blocks and optional finally block
     */
    class TryNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> tryBlock;                      // Try block body
        std::vector<std::unique_ptr<CatchNode>> catchBlocks;    // Catch clauses
        std::unique_ptr<ASTNode> finallyBlock;                  // Optional finally block

    public:
        explicit TryNode(std::unique_ptr<ASTNode> tryBlock,
                        std::vector<std::unique_ptr<CatchNode>> catchBlocks,
                        std::unique_ptr<ASTNode> finallyBlock = nullptr,
                        const SourceLocation& loc = SourceLocation());

        ASTNode* getTryBlock() const;
        const std::vector<std::unique_ptr<CatchNode>>& getCatchBlocks() const;
        ASTNode* getFinallyBlock() const;

        void setTryBlock(std::unique_ptr<ASTNode> block);
        void addCatchBlock(std::unique_ptr<CatchNode> catchBlock);
        void setFinallyBlock(std::unique_ptr<ASTNode> block);

        bool hasCatchBlocks() const;
        bool hasFinallyBlock() const;
        size_t getCatchBlockCount() const;

        value::Value accept(ASTVisitor<value::Value>& visitor) override;
    };
}
