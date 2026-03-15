#include "MatchCaseNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    // Type pattern
    MatchCaseNode::MatchCaseNode(const std::string& typeName, const std::string& bindingName,
                                 std::unique_ptr<ASTNode> body, const SourceLocation& loc)
        : ASTNode(loc), patternKind(PatternKind::TYPE),
          typeName(typeName), bindingName(bindingName), body(std::move(body))
    {
    }

    // Value pattern
    MatchCaseNode::MatchCaseNode(std::unique_ptr<ASTNode> valueExpr,
                                 std::unique_ptr<ASTNode> body, const SourceLocation& loc)
        : ASTNode(loc), patternKind(PatternKind::VALUE),
          valueExpression(std::move(valueExpr)), body(std::move(body))
    {
    }

    // Null pattern
    MatchCaseNode::MatchCaseNode(std::unique_ptr<ASTNode> body,
                                 PatternKind kind, const SourceLocation& loc)
        : ASTNode(loc), patternKind(kind), body(std::move(body))
    {
    }

    PatternKind MatchCaseNode::getPatternKind() const { return patternKind; }
    const std::string& MatchCaseNode::getTypeName() const { return typeName; }
    const std::string& MatchCaseNode::getBindingName() const { return bindingName; }
    ASTNode* MatchCaseNode::getValueExpression() const { return valueExpression.get(); }
    ASTNode* MatchCaseNode::getBody() const { return body.get(); }

    void MatchCaseNode::setBody(std::unique_ptr<ASTNode> newBody)
    {
        body = std::move(newBody);
    }

    Value MatchCaseNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMatchCaseNode(this);
    }

    std::unique_ptr<ASTNode> MatchCaseNode::clone() const
    {
        switch (patternKind) {
            case PatternKind::TYPE:
                return std::make_unique<MatchCaseNode>(
                    typeName, bindingName,
                    ast::utils::cloneNodeSafe(body), location);
            case PatternKind::VALUE:
                return std::make_unique<MatchCaseNode>(
                    ast::utils::cloneNodeSafe(valueExpression),
                    ast::utils::cloneNodeSafe(body), location);
            case PatternKind::NULL_PATTERN:
                return std::make_unique<MatchCaseNode>(
                    ast::utils::cloneNodeSafe(body),
                    PatternKind::NULL_PATTERN, location);
        }
        return nullptr;
    }
}
