#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class SuperConstructorCallNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> arguments;

    public:
        explicit SuperConstructorCallNode(
            std::vector<std::unique_ptr<ASTNode>> args,
            const SourceLocation& loc = SourceLocation());

        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        std::vector<std::unique_ptr<ASTNode>>& getArguments();
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
