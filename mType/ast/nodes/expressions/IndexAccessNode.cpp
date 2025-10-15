#include "IndexAccessNode.hpp"

namespace ast::nodes::expressions
{
    IndexAccessNode::IndexAccessNode(std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> index, const SourceLocation& loc)
        : ASTNode(loc), collection(std::move(collection)), index(std::move(index)) {}

    ASTNode* IndexAccessNode::getCollection() const
    {
        return collection.get();
    }

    ASTNode* IndexAccessNode::getIndex() const
    {
        return index.get();
    }

    void IndexAccessNode::setCollection(std::unique_ptr<ASTNode> newCollection)
    {
        collection = std::move(newCollection);
    }

    void IndexAccessNode::setIndex(std::unique_ptr<ASTNode> newIndex)
    {
        index = std::move(newIndex);
    }

    std::unique_ptr<ASTNode> IndexAccessNode::transferCollectionOwnership()
    {
        return std::move(collection);
    }

    std::unique_ptr<ASTNode> IndexAccessNode::transferIndexOwnership()
    {
        return std::move(index);
    }

    Value IndexAccessNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitIndexAccessNode(this);
    }

    std::unique_ptr<ASTNode> IndexAccessNode::clone() const
    {
        auto clonedCollection = collection ? collection->clone() : nullptr;
        auto clonedIndex = index ? index->clone() : nullptr;
        return std::make_unique<IndexAccessNode>(
            std::move(clonedCollection),
            std::move(clonedIndex),
            location
        );
    }
}