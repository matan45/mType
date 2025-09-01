#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::statements
{
    class CaseNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> value;
        std::vector<std::unique_ptr<ASTNode>> statements;

    public:
        explicit CaseNode(std::unique_ptr<ASTNode> caseValue, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), value(std::move(caseValue)) {}

        std::unique_ptr<ASTNode> getValue() const { return value.get(); }
        const std::vector<std::unique_ptr<ASTNode>>& getStatements() const { return statements; }

        void setValue(std::unique_ptr<ASTNode> caseValue) { value = std::move(caseValue); }
        void addStatement(std::unique_ptr<ASTNode> statement) { statements.push_back(std::move(statement)); }

        size_t getStatementCount() const { return statements.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitCaseNode(this);
        }
    };
}
