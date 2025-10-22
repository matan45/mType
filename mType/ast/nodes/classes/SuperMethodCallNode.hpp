#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class SuperMethodCallNode : public ASTNode
    {
    private:
        std::string methodName;
        std::vector<std::unique_ptr<ASTNode>> arguments;

    public:
        explicit SuperMethodCallNode(
            const std::string& method,
            std::vector<std::unique_ptr<ASTNode>> args,
            const SourceLocation& loc = SourceLocation());

        const std::string& getMethodName() const;
        void setMethodName(const std::string& method);

        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
