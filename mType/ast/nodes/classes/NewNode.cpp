#include "NewNode.hpp"
#include <cstddef>

namespace ast::nodes::classes
{
    NewNode::NewNode(const std::string& clsName, std::vector<std::unique_ptr<ASTNode>> args, const SourceLocation& loc)
        : ASTNode(loc), className(clsName), arguments(std::move(args))
    {
    }

    const std::string& NewNode::getClassName() const
    {
        return className;
    }

    const std::vector<std::unique_ptr<ASTNode>>& NewNode::getArguments() const
    {
        return arguments;
    }

    void NewNode::setClassName(const std::string& clsName)
    {
        className = clsName;
    }

    void NewNode::setArguments(std::vector<std::unique_ptr<ASTNode>> args)
    {
        arguments = std::move(args);
    }

    void NewNode::addArgument(std::unique_ptr<ASTNode> arg)
    {
        arguments.push_back(std::move(arg));
    }

    size_t NewNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value NewNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitNewNode(this);
    }

    std::unique_ptr<ASTNode> NewNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }
        auto cloned = std::make_unique<NewNode>(className, std::move(clonedArgs), location);
        cloned->isStackAllocated = isStackAllocated;
        return cloned;
    }
}
