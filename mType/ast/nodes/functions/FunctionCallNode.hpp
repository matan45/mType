#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::functions
{
    class FunctionCallNode : public ASTNode
    {
    private:
        std::string functionName;
        std::vector<std::unique_ptr<ASTNode>> arguments;

    public:
        explicit FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                         const SourceLocation& loc = SourceLocation());

        const std::string& getFunctionName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;

        void setFunctionName(const std::string& funcName);
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args);

        void addArgument(std::unique_ptr<ASTNode> arg);
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
