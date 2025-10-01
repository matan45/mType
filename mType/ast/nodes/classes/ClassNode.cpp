#include "ClassNode.hpp"

namespace ast::nodes::classes
{
    // Backward compatibility constructor
    ClassNode::ClassNode(const std::string& name, const SourceLocation& loc)
        : ASTNode(loc), className(name)
    {
    }

    ClassNode::ClassNode(const std::string& name,
                         const std::vector<GenericTypeParameter>& generics,
                         const std::vector<std::string>& interfaces,
                         const SourceLocation& loc)
        : ASTNode(loc), className(name), genericParameters(generics),implementedInterfaces(interfaces)
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

    const std::vector<std::string>& ClassNode::getImplementedInterfaces() const
    {
        return implementedInterfaces;
    }

    const std::vector<GenericTypeParameter>& ClassNode::getGenericParameters() const
    {
        return genericParameters;
    }

    void ClassNode::setGenericParameters(const std::vector<GenericTypeParameter>& generics)
    {
        genericParameters = generics;
    }

    void ClassNode::addGenericParameter(const GenericTypeParameter& param)
    {
        genericParameters.push_back(param);
    }

    size_t ClassNode::getGenericParameterCount() const
    {
        return genericParameters.size();
    }

    bool ClassNode::isGeneric() const
    { return !genericParameters.empty(); }

    std::string ClassNode::getFullClassName() const
    {
        std::string fullName = className;
        if (!genericParameters.empty()) {
            fullName += "<";
            for (size_t i = 0; i < genericParameters.size(); ++i) {
                if (i > 0) fullName += ", ";
                fullName += genericParameters[i].toString();
            }
            fullName += ">";
        }
        return fullName;
    }

    Value ClassNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitClassNode(this);
    }
}
