#include "TryNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    TryNode::TryNode(std::unique_ptr<ASTNode> tryBlock,
                     std::vector<std::unique_ptr<CatchNode>> catchBlocks,
                     std::unique_ptr<ASTNode> finallyBlock,
                     const SourceLocation& loc)
        : ASTNode(loc), tryBlock(std::move(tryBlock)),
          catchBlocks(std::move(catchBlocks)), finallyBlock(std::move(finallyBlock))
    {
    }

    ASTNode* TryNode::getTryBlock() const
    {
        return tryBlock.get();
    }

    const std::vector<std::unique_ptr<CatchNode>>& TryNode::getCatchBlocks() const
    {
        return catchBlocks;
    }

    ASTNode* TryNode::getFinallyBlock() const
    {
        return finallyBlock.get();
    }

    void TryNode::setTryBlock(std::unique_ptr<ASTNode> block)
    {
        tryBlock = std::move(block);
    }

    void TryNode::addCatchBlock(std::unique_ptr<CatchNode> catchBlock)
    {
        catchBlocks.push_back(std::move(catchBlock));
    }

    void TryNode::setFinallyBlock(std::unique_ptr<ASTNode> block)
    {
        finallyBlock = std::move(block);
    }

    bool TryNode::hasCatchBlocks() const
    {
        return !catchBlocks.empty();
    }

    bool TryNode::hasFinallyBlock() const
    {
        return finallyBlock != nullptr;
    }

    size_t TryNode::getCatchBlockCount() const
    {
        return catchBlocks.size();
    }

    value::Value TryNode::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitTryNode(this);
    }

    std::unique_ptr<ASTNode> TryNode::clone() const
    {
        auto clonedTryBlock = ast::utils::cloneNodeSafe(tryBlock);
        auto clonedCatchBlocks = ast::utils::cloneNodeVector<CatchNode>(catchBlocks);
        auto clonedFinallyBlock = ast::utils::cloneNodeSafe(finallyBlock);

        return std::make_unique<TryNode>(std::move(clonedTryBlock), std::move(clonedCatchBlocks),
                                         std::move(clonedFinallyBlock), location);
    }
}
