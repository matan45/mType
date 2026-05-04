#include "FunctionCallNode.hpp"
#include <cstddef>

namespace ast::nodes::functions
{
    FunctionCallNode::FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                                       const std::vector<std::string>& genericArgs,
                                       const SourceLocation& loc)
        : ASTNode(loc), functionName(funcName), arguments(std::move(args)), genericTypeArguments(genericArgs)
    {
    }

    // Backward compatibility constructor
    FunctionCallNode::FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                                       const SourceLocation& loc)
        : ASTNode(loc), functionName(funcName), arguments(std::move(args)), genericTypeArguments()
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

    const std::vector<std::string>& FunctionCallNode::getGenericTypeArguments() const
    {
        return genericTypeArguments;
    }

    bool FunctionCallNode::hasGenericTypeArguments() const
    {
        return !genericTypeArguments.empty();
    }

    void FunctionCallNode::setFunctionName(const std::string& funcName)
    {
        functionName = funcName;
    }

    void FunctionCallNode::setArguments(std::vector<std::unique_ptr<ASTNode>> args)
    {
        arguments = std::move(args);
    }

    void FunctionCallNode::setGenericTypeArguments(const std::vector<std::string>& genericArgs)
    {
        genericTypeArguments = genericArgs;
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

    std::unique_ptr<ASTNode> FunctionCallNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }
        return std::make_unique<FunctionCallNode>(
            functionName,
            std::move(clonedArgs),
            genericTypeArguments,
            location
        );
    }
}
