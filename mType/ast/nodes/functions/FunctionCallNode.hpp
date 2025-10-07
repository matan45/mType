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
        std::vector<std::string> genericTypeArguments; // For function<Type>() calls

    public:
        // Primary constructor with generic type arguments
        explicit FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                         const std::vector<std::string>& genericArgs,
                         const SourceLocation& loc = SourceLocation());

        // Backward compatibility constructor without generic type arguments
        explicit FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                         const SourceLocation& loc = SourceLocation());

        const std::string& getFunctionName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        const std::vector<std::string>& getGenericTypeArguments() const;
        bool hasGenericTypeArguments() const;

        void setFunctionName(const std::string& funcName);
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args);
        void setGenericTypeArguments(const std::vector<std::string>& genericArgs);

        void addArgument(std::unique_ptr<ASTNode> arg);
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
