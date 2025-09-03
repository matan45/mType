#include "FunctionCallNode.hpp"

namespace ast::nodes::functions
{
    FunctionCallNode::FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                                       const SourceLocation& loc)
        : ASTNode(loc), functionName(funcName), arguments(std::move(args))
    {
    }

    const std::string& FunctionCallNode::getFunctionName() const
    {
        return functionName;
    }

    const std::vector<std::unique_ptr<ASTNode>>& FunctionCallNode::getArguments() const
    {
        return arguments;
    }

    void FunctionCallNode::setFunctionName(const std::string& funcName)
    {
        functionName = funcName;
    }

    void FunctionCallNode::setArguments(std::vector<std::unique_ptr<ASTNode>> args)
    {
        arguments = std::move(args);
    }

    void FunctionCallNode::addArgument(std::unique_ptr<ASTNode> arg)
    {
        arguments.push_back(std::move(arg));
    }

    size_t FunctionCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value FunctionCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFunctionCallNode(this);
    }
}
