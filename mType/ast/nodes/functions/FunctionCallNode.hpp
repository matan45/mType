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
        std::vector<std::string> qualifiedName; // For namespace::function calls

    public:
        explicit FunctionCallNode(const std::string& funcName, std::vector<std::unique_ptr<ASTNode>> args,
                         const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), functionName(funcName), arguments(std::move(args)) {}

        explicit FunctionCallNode(const std::vector<std::string>& qualName, std::vector<std::unique_ptr<ASTNode>> args,
                         const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), qualifiedName(qualName), arguments(std::move(args)) {
            if (!qualName.empty()) {
                functionName = qualName.back();
            }
        }

        const std::string& getFunctionName() const { return functionName; }
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const { return arguments; }
        const std::vector<std::string>& getQualifiedName() const { return qualifiedName; }

        void setFunctionName(const std::string& funcName) { functionName = funcName; }
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args) { arguments = std::move(args); }
        void setQualifiedName(const std::vector<std::string>& qualName) { 
            qualifiedName = qualName;
            if (!qualName.empty()) {
                functionName = qualName.back();
            }
        }

        void addArgument(std::unique_ptr<ASTNode> arg) { arguments.push_back(std::move(arg)); }
        size_t getArgumentCount() const { return arguments.size(); }
        bool isQualified() const { return !qualifiedName.empty(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitFunctionCallNode(this);
        }
    };
}
