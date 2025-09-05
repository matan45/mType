#include "MethodNode.hpp"

namespace ast::nodes::classes
{
    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           std::vector<std::pair<std::string, ValueType>> params, std::shared_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), returnType(retType), parameters(std::move(params)),
          body(std::move(methodBody)), isStatic(isStaticMethod)
    {
    }

    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           std::vector<std::pair<std::string, ValueType>> params, std::unique_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), returnType(retType), parameters(std::move(params)),
          body(std::move(methodBody)), isStatic(isStaticMethod) // unique_ptr converts to shared_ptr
    {
    }

    const std::string& MethodNode::getName() const
    {
        return name;
    }

    ValueType MethodNode::getReturnType() const
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, ValueType>>& MethodNode::getParameters() const
    {
        return parameters;
    }

    std::shared_ptr<ASTNode> MethodNode::getBody() const
    {
        return body;
    }

    ASTNode* MethodNode::getBodyPtr() const noexcept
    {
        return body.get();
    }

    bool MethodNode::getIsStatic() const
    {
        return isStatic;
    }

    void MethodNode::setName(const std::string& methodName)
    {
        name = methodName;
    }

    void MethodNode::setReturnType(ValueType retType)
    {
        returnType = retType;
    }

    void MethodNode::setParameters(std::vector<std::pair<std::string, ValueType>> params)
    {
        parameters = std::move(params);
    }

    void MethodNode::setBody(std::shared_ptr<ASTNode> methodBody)
    {
        body = std::move(methodBody);
    }

    void MethodNode::setIsStatic(bool isStaticMethod)
    {
        isStatic = isStaticMethod;
    }

    size_t MethodNode::getParameterCount() const
    {
        return parameters.size();
    }

    Value MethodNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMethodNode(this);
    }
}
