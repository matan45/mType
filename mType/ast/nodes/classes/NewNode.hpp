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
        // MYT-134: set by EscapeAnalysisPass when this allocation never escapes its
        // enclosing function. Compiler emits NEW_STACK instead of NEW_OBJECT.
        bool isStackAllocated = false;

    public:
        explicit NewNode(const std::string& clsName, std::vector<std::unique_ptr<ASTNode>> args,
                         const SourceLocation& loc = SourceLocation());

        const std::string& getClassName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;

        void setClassName(const std::string& clsName);
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args);

        void addArgument(std::unique_ptr<ASTNode> arg);
        size_t getArgumentCount() const;

        bool getIsStackAllocated() const { return isStackAllocated; }
        void setIsStackAllocated(bool v) { isStackAllocated = v; }

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
