#include "ClassNode.hpp"

namespace ast::nodes::classes
{
    ClassNode::ClassNode(const std::string& name, const SourceLocation& loc)
        : ASTNode(loc), className(name)
    {
    }

    const std::string& ClassNode::getClassName() const
    {
        return className;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getFields() const
    {
        return fields;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getConstructors() const
    {
        return constructors;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getMethods() const
    {
        return methods;
    }

    void ClassNode::setClassName(const std::string& name)
    {
        className = name;
    }

    void ClassNode::addField(std::unique_ptr<ASTNode> field)
    {
        fields.push_back(std::move(field));
    }

    void ClassNode::addConstructor(std::unique_ptr<ASTNode> constructor)
    {
        constructors.push_back(std::move(constructor));
    }

    void ClassNode::addMethod(std::unique_ptr<ASTNode> method)
    {
        methods.push_back(std::move(method));
    }

    size_t ClassNode::getFieldCount() const
    {
        return fields.size();
    }

    size_t ClassNode::getConstructorCount() const
    {
        return constructors.size();
    }

    size_t ClassNode::getMethodCount() const
    {
        return methods.size();
    }

    Value ClassNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitClassNode(this);
    }
}
