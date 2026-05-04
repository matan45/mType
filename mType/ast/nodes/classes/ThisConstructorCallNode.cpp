#include "ThisConstructorCallNode.hpp"
#include <cstddef>

namespace ast::nodes::classes
{
    ThisConstructorCallNode::ThisConstructorCallNode(
        std::vector<std::unique_ptr<ASTNode>> args,
        const SourceLocation& loc)
        : ASTNode(loc), arguments(std::move(args))
    {
    }

    const std::vector<std::unique_ptr<ASTNode>>& ThisConstructorCallNode::getArguments() const
    {
        return arguments;
    }

    size_t ThisConstructorCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value ThisConstructorCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitThisConstructorCallNode(this);
    }

    std::unique_ptr<ASTNode> ThisConstructorCallNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }
        return std::make_unique<ThisConstructorCallNode>(std::move(clonedArgs), location);
    }
}
