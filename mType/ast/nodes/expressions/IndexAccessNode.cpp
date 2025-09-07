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

    Value IndexAccessNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitIndexAccessNode(this);
    }
}