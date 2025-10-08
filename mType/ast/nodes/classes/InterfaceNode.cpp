#include "InterfaceNode.hpp"

namespace ast::nodes::classes
{
    InterfaceNode::InterfaceNode(const std::string& interfaceName, const std::vector<GenericTypeParameter>& generics,
                                 const SourceLocation& loc) : ASTNode(loc), name(interfaceName),
                                                              genericParameters(generics), finalInterface(false)
    {
    }

    void InterfaceNode::addMethod(std::unique_ptr<ASTNode> method)
    {
        methods.push_back(std::move(method));
    }

    const std::string& InterfaceNode::getName() const
    {
        return name;
    }

    const std::vector<GenericTypeParameter>& InterfaceNode::getGenericParameters() const
    {
        return genericParameters;
    }

    bool InterfaceNode::isGeneric() const
    {
        return !genericParameters.empty();
    }

    void InterfaceNode::addExtendedInterface(const std::string& interfaceName)
    {
        extendsInterfaces.push_back(interfaceName);
    }

    const std::vector<std::string>& InterfaceNode::getExtendedInterfaces() const
    {
        return extendsInterfaces;
    }

    bool InterfaceNode::extendsInterface(const std::string& interfaceName) const
    {
        for (const auto& extendedInterface : extendsInterfaces)
        {
            if (extendedInterface == interfaceName)
            {
                return true;
            }
        }
        return false;
    }

    std::string InterfaceNode::getFullInterfaceName() const
    {
        if (genericParameters.empty())
        {
            return name;
        }

        std::string fullName = name + "<";
        for (size_t i = 0; i < genericParameters.size(); ++i)
        {
            if (i > 0) fullName += ", ";
            fullName += genericParameters[i].name;
        }
        fullName += ">";
        return fullName;
    }

    const std::vector<std::unique_ptr<ASTNode>>& InterfaceNode::getMethods() const
    {
        return methods;
    }

    bool InterfaceNode::isFinal() const
    {
        return finalInterface;
    }

    void InterfaceNode::setFinal(bool isFinal)
    {
        finalInterface = isFinal;
    }

    Value InterfaceNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitInterfaceNode(this);
    }
}
