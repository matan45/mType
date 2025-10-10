#include "CatchNode.hpp"

namespace ast::nodes::statements
{
    CatchNode::CatchNode(const std::string& exceptionType, const std::string& variableName,
                         std::unique_ptr<ASTNode> body, const SourceLocation& loc)
        : ASTNode(loc), exceptionType(exceptionType), variableName(variableName), body(std::move(body))
    {
    }

    const std::string& CatchNode::getExceptionType() const
    {
        return exceptionType;
    }

    const std::string& CatchNode::getVariableName() const
    {
        return variableName;
    }

    ASTNode* CatchNode::getBody() const
    {
        return body.get();
    }

    void CatchNode::setExceptionType(const std::string& type)
    {
        exceptionType = type;
    }

    void CatchNode::setVariableName(const std::string& name)
    {
        variableName = name;
    }

    void CatchNode::setBody(std::unique_ptr<ASTNode> catchBody)
    {
        body = std::move(catchBody);
    }

    value::Value CatchNode::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitCatchNode(this);
    }
}
