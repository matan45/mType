#include "SuperConstructorCallNode.hpp"
#include <cstddef>

namespace ast::nodes::classes
{
    SuperConstructorCallNode::SuperConstructorCallNode(
        std::vector<std::unique_ptr<ASTNode>> args,
        const SourceLocation& loc)
        : ASTNode(loc), arguments(std::move(args))
    {
    }

    const std::vector<std::unique_ptr<ASTNode>>& SuperConstructorCallNode::getArguments() const
    {
        return arguments;
    }

    size_t SuperConstructorCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value SuperConstructorCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSuperConstructorCallNode(this);
    }

    std::unique_ptr<ASTNode> SuperConstructorCallNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }
        return std::make_unique<SuperConstructorCallNode>(std::move(clonedArgs), location);
    }
}
