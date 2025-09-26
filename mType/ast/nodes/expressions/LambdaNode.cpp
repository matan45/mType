#include "LambdaNode.hpp"

namespace ast::nodes::expressions
{
    LambdaNode::LambdaNode(const std::vector<Parameter>& params, std::unique_ptr<ASTNode> lambdaBody,
                           const SourceLocation& loc, BodyType type)
        : ASTNode(loc), parameters(params), body(std::move(lambdaBody)), bodyType(type)
    {
    }

    const std::vector<Parameter>& LambdaNode::getParameters() const
    {
        return parameters;
    }

    ASTNode* LambdaNode::getBody() const
    {
        return body.get();
    }

    BodyType LambdaNode::getBodyType() const
    {
        return bodyType;
    }

    void LambdaNode::setTargetInterface(const std::string& interface)
    {
        targetInterface = interface;
    }

    void LambdaNode::setTargetMethod(const std::string& method)
    {
        targetMethod = method;
    }

    bool LambdaNode::isExpressionLambda() const
    {
        return bodyType == BodyType::EXPRESSION;
    }

    bool LambdaNode::isBlockLambda() const
    {
        return bodyType == BodyType::BLOCK;
    }

    Value LambdaNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitLambdaNode(this);
    }
}
