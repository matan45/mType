#include "LambdaNode.hpp"

namespace ast::nodes::expressions
{
    LambdaNode::LambdaNode(const std::vector<Parameter>& params, std::unique_ptr<ASTNode> lambdaBody,
                           const SourceLocation& loc, BodyType type, bool async)
        : ASTNode(loc), parameters(params), body(std::move(lambdaBody)), bodyType(type), isAsync(async)
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

    std::unique_ptr<ASTNode> LambdaNode::clone() const
    {
        // Clone parameters (deep copy of Parameter structs with GenericType)
        std::vector<Parameter> clonedParams;
        clonedParams.reserve(parameters.size());
        for (const auto& param : parameters) {
            std::shared_ptr<GenericType> clonedType = param.type ? std::make_shared<GenericType>(*param.type) : nullptr;
            clonedParams.emplace_back(param.name, clonedType);
        }

        // Clone body
        std::unique_ptr<ASTNode> clonedBody = body ? body->clone() : nullptr;

        auto clonedLambda = std::make_unique<LambdaNode>(clonedParams, std::move(clonedBody), location, bodyType, isAsync);

        clonedLambda->setTargetInterface(targetInterface);
        clonedLambda->setTargetMethod(targetMethod);

        return clonedLambda;
    }
}
