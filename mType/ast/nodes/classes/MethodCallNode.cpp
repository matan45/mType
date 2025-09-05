#include "MethodCallNode.hpp"

namespace ast::nodes::classes
{
    MethodCallNode::MethodCallNode(std::unique_ptr<ASTNode> obj, const std::string& method,
                                   std::vector<std::unique_ptr<ASTNode>> args, bool isStatic, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), methodName(method),
          arguments(std::move(args)), isStaticCall(isStatic)
    {
    }

    ASTNode* MethodCallNode::getObject() const
    {
        return object.get();
    }

    const std::string& MethodCallNode::getMethodName() const
    {
        return methodName;
    }

    const std::vector<std::unique_ptr<ASTNode>>& MethodCallNode::getArguments() const
    {
        return arguments;
    }

    bool MethodCallNode::getIsStaticCall() const
    {
        return isStaticCall;
    }

    void MethodCallNode::setObject(std::unique_ptr<ASTNode> obj)
    {
        object = std::move(obj);
    }

    void MethodCallNode::setMethodName(const std::string& method)
    {
        methodName = method;
    }

    void MethodCallNode::setArguments(std::vector<std::unique_ptr<ASTNode>> args)
    {
        arguments = std::move(args);
    }

    void MethodCallNode::setIsStaticCall(bool isStatic)
    {
        isStaticCall = isStatic;
    }

    void MethodCallNode::addArgument(std::unique_ptr<ASTNode> arg)
    {
        arguments.push_back(std::move(arg));
    }

    size_t MethodCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value MethodCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMethodCallNode(this);
    }
}
