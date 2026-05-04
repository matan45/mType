#include "SuperMethodCallNode.hpp"
#include <cstddef>

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

    size_t SuperMethodCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value SuperMethodCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSuperMethodCallNode(this);
    }

    std::unique_ptr<ASTNode> SuperMethodCallNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }
        return std::make_unique<SuperMethodCallNode>(methodName, std::move(clonedArgs), location);
    }
}
