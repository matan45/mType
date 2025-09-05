#include "FieldNode.hpp"

namespace ast::nodes::classes
{
    FieldNode::FieldNode(const std::string& fieldName, ValueType fieldType, std::unique_ptr<ASTNode> initValue,
                         bool isStaticField, bool isFinalField, const SourceLocation& loc)
        : ASTNode(loc), name(fieldName), type(fieldType), initialValue(std::move(initValue)),
          isStatic(isStaticField), isFinal(isFinalField)
    {
    }

    const std::string& FieldNode::getName() const
    {
        return name;
    }

    ValueType FieldNode::getType() const
    {
        return type;
    }

    ASTNode* FieldNode::getInitialValue() const
    {
        return initialValue.get();
    }

    bool FieldNode::getIsStatic() const
    {
        return isStatic;
    }

    bool FieldNode::getIsFinal() const
    {
        return isFinal;
    }

    void FieldNode::setName(const std::string& fieldName)
    {
        name = fieldName;
    }

    void FieldNode::setType(ValueType fieldType)
    {
        type = fieldType;
    }

    void FieldNode::setInitialValue(std::unique_ptr<ASTNode> initValue)
    {
        initialValue = std::move(initValue);
    }

    void FieldNode::setIsStatic(bool isStaticField)
    {
        isStatic = isStaticField;
    }

    void FieldNode::setIsFinal(bool isFinalField)
    {
        isFinal = isFinalField;
    }

    bool FieldNode::hasInitialValue() const
    {
        return initialValue != nullptr;
    }

    Value FieldNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFieldNode(this);
    }
}
