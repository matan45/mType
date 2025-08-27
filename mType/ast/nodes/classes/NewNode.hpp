#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class NewNode : public ASTNode
    {
    private:
        std::string className;
        std::vector<std::unique_ptr<ASTNode>> arguments;

    public:
        explicit NewNode(const std::string& clsName, std::vector<std::unique_ptr<ASTNode>> args,
                const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), className(clsName), arguments(std::move(args))
        {
        }

        const std::string& getClassName() const { return className; }
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const { return arguments; }

        void setClassName(const std::string& clsName) { className = clsName; }
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args) { arguments = std::move(args); }

        void addArgument(std::unique_ptr<ASTNode> arg) { arguments.push_back(std::move(arg)); }
        size_t getArgumentCount() const { return arguments.size(); }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitNewNode(this);
        }
    };
}
