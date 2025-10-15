#include "IndexAssignmentNode.hpp"

namespace ast::nodes::statements
{
    IndexAssignmentNode::IndexAssignmentNode(std::shared_ptr<ASTNode> obj, std::shared_ptr<ASTNode> idx,
                                             std::shared_ptr<ASTNode> val, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), index(std::move(idx)), value(std::move(val))
    {
    }

    IndexAssignmentNode::IndexAssignmentNode(std::unique_ptr<ASTNode> obj, std::unique_ptr<ASTNode> idx,
                                             std::unique_ptr<ASTNode> val, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), index(std::move(idx)), value(std::move(val))
    {
    }

    ASTNode* IndexAssignmentNode::getObject() const noexcept
    {
        return object.get();
    }

    ASTNode* IndexAssignmentNode::getIndex() const noexcept
    {
        return index.get();
    }

    ASTNode* IndexAssignmentNode::getValue() const noexcept
    {
        return value.get();
    }

    std::shared_ptr<ASTNode> IndexAssignmentNode::getObjectShared() const
    {
        return object;
    }

    std::shared_ptr<ASTNode> IndexAssignmentNode::getIndexShared() const
    {
        return index;
    }

    std::shared_ptr<ASTNode> IndexAssignmentNode::getValueShared() const
    {
        return value;
    }

    void IndexAssignmentNode::setObject(std::shared_ptr<ASTNode> obj)
    {
        object = std::move(obj);
    }

    void IndexAssignmentNode::setIndex(std::shared_ptr<ASTNode> idx)
    {
        index = std::move(idx);
    }

    void IndexAssignmentNode::setValue(std::shared_ptr<ASTNode> val)
    {
        value = std::move(val);
    }

    Value IndexAssignmentNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitIndexAssignmentNode(this);
    }

    std::unique_ptr<ASTNode> IndexAssignmentNode::clone() const
    {
        std::shared_ptr<ASTNode> clonedObject = object ? std::shared_ptr<ASTNode>(object->clone()) : nullptr;
        std::shared_ptr<ASTNode> clonedIndex = index ? std::shared_ptr<ASTNode>(index->clone()) : nullptr;
        std::shared_ptr<ASTNode> clonedValue = value ? std::shared_ptr<ASTNode>(value->clone()) : nullptr;

        return std::make_unique<IndexAssignmentNode>(clonedObject, clonedIndex, clonedValue, location);
    }
}