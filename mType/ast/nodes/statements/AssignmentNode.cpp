#include "AssignmentNode.hpp"

namespace ast::nodes::statements
{
    AssignmentNode::AssignmentNode(const std::string& varName, std::unique_ptr<ASTNode> val, ValueType type,
        const std::string& clsName, bool isFinalVar, bool isStaticVar, const SourceLocation& loc)
    : ASTNode(loc), variableName(varName), value(std::move(val)), variableType(type),
           className(clsName), isFinal(isFinalVar), isStatic(isStaticVar),
           visibility(VisibilityModifier::PUBLIC)  // Default to PUBLIC
    {
    }

    const std::string& AssignmentNode::getVariableName() const
    {
        return variableName;
    }

    ASTNode* AssignmentNode::getValue() const
    {
        return value.get();
    }

    value::ValueType AssignmentNode::getVariableType() const
    {
        return variableType;
    }

    const std::string& AssignmentNode::getClassName() const
    {
        return className;
    }

    bool AssignmentNode::getIsFinal() const
    {
        return isFinal;
    }

    bool AssignmentNode::getIsStatic() const
    {
        return isStatic;
    }

    VisibilityModifier AssignmentNode::getVisibility() const
    {
        return visibility;
    }

    void AssignmentNode::setVariableName(const std::string& varName)
    {
        variableName = varName;
    }

    void AssignmentNode::setValue(std::unique_ptr<ASTNode> val)
    {
        value = std::move(val);
    }

    void AssignmentNode::setVariableType(value::ValueType type)
    {
        variableType = type;
    }

    void AssignmentNode::setClassName(const std::string& clsName)
    {
        className = clsName;
    }

    void AssignmentNode::setIsFinal(bool isFinalVar)
    {
        isFinal = isFinalVar;
    }

    void AssignmentNode::setIsStatic(bool isStaticVar)
    {
        isStatic = isStaticVar;
    }

    void AssignmentNode::setNullableType(bool isNullable)
    {
        nullableType = isNullable;
    }

    bool AssignmentNode::isNullableType() const
    {
        return nullableType;
    }

    void AssignmentNode::setVisibility(VisibilityModifier vis)
    {
        visibility = vis;
    }

    Value AssignmentNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitAssignmentNode(this);
    }

    std::unique_ptr<ASTNode> AssignmentNode::clone() const
    {
        std::unique_ptr<ASTNode> clonedValue = value ? value->clone() : nullptr;

        auto clonedAssignment = std::make_unique<AssignmentNode>(
            variableName, std::move(clonedValue), variableType, className, isFinal, isStatic, location
        );

        clonedAssignment->setVisibility(visibility);
        clonedAssignment->setNullableType(nullableType);

        return clonedAssignment;
    }
}
