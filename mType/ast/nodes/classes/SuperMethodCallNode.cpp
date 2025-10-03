#include "SuperMethodCallNode.hpp"

namespace ast::nodes::classes
{
    SuperMethodCallNode::SuperMethodCallNode(
        const std::string& method,
        std::vector<std::unique_ptr<ASTNode>> args,
        const SourceLocation& loc)
        : ASTNode(loc), methodName(method), arguments(std::move(args))
    {
    }

    const std::string& SuperMethodCallNode::getMethodName() const
    {
        return methodName;
    }

    void SuperMethodCallNode::setMethodName(const std::string& method)
    {
        methodName = method;
    }

    const std::vector<std::unique_ptr<ASTNode>>& SuperMethodCallNode::getArguments() const
    {
        return arguments;
    }

    std::vector<std::unique_ptr<ASTNode>>& SuperMethodCallNode::getArguments()
    {
        return arguments;
    }

    size_t SuperMethodCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value SuperMethodCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSuperMethodCallNode(this);
    }
}
