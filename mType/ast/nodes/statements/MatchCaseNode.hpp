#pragma once
#include "../../ASTNode.hpp"
#include <memory>
#include <string>

namespace ast::nodes::statements
{
    enum class PatternKind { VALUE, TYPE, NULL_PATTERN };

    class MatchCaseNode : public ASTNode
    {
    private:
        PatternKind patternKind;
        std::string typeName;                     // For type patterns: "int", "Animal"
        std::string bindingName;                  // For type patterns: variable name
        std::unique_ptr<ASTNode> valueExpression; // For value patterns: literal expression
        std::unique_ptr<ASTNode> body;

    public:
        // Type pattern constructor
        MatchCaseNode(const std::string& typeName, const std::string& bindingName,
                      std::unique_ptr<ASTNode> body, const SourceLocation& loc = SourceLocation());

        // Value pattern constructor
        MatchCaseNode(std::unique_ptr<ASTNode> valueExpr,
                      std::unique_ptr<ASTNode> body, const SourceLocation& loc = SourceLocation());

        // Null pattern constructor
        explicit MatchCaseNode(std::unique_ptr<ASTNode> body,
                               PatternKind kind, const SourceLocation& loc = SourceLocation());

        PatternKind getPatternKind() const;
        const std::string& getTypeName() const;
        const std::string& getBindingName() const;
        ASTNode* getValueExpression() const;
        ASTNode* getBody() const;

        void setBody(std::unique_ptr<ASTNode> newBody);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
