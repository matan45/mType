#pragma once
#include "../../ASTNode.hpp"
#include <cstddef>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class ThisConstructorCallNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> arguments;

    public:
        explicit ThisConstructorCallNode(
            std::vector<std::unique_ptr<ASTNode>> args,
            const SourceLocation& loc = SourceLocation());

        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
