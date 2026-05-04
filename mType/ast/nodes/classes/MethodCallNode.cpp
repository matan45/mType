#include "MethodCallNode.hpp"
#include <cstddef>

namespace ast::nodes::classes
{
    MethodCallNode::MethodCallNode(std::unique_ptr<ASTNode> obj, const std::string& method,
                                   std::vector<std::unique_ptr<ASTNode>> args, bool isStatic,
                                   const std::vector<std::string>& genericArgs, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), methodName(method),
          arguments(std::move(args)), isStaticCall(isStatic), genericTypeArguments(genericArgs)
    {
    }

    ASTNode* MethodCallNode::getObject() const
    {
        return object.get();
    }

    const std::string& MethodCallNode::getMethodName() const
    {
        return methodName;
    }

    const std::vector<std::unique_ptr<ASTNode>>& MethodCallNode::getArguments() const
    {
        return arguments;
    }

    bool MethodCallNode::getIsStaticCall() const
    {
        return isStaticCall;
    }

    const std::vector<std::string>& MethodCallNode::getGenericTypeArguments() const
    {
        return genericTypeArguments;
    }

    bool MethodCallNode::hasGenericTypeArguments() const
    {
        return !genericTypeArguments.empty();
    }

    void MethodCallNode::setObject(std::unique_ptr<ASTNode> obj)
    {
        object = std::move(obj);
    }

    void MethodCallNode::setMethodName(const std::string& method)
    {
        methodName = method;
    }

    void MethodCallNode::setArguments(std::vector<std::unique_ptr<ASTNode>> args)
    {
        arguments = std::move(args);
    }

    void MethodCallNode::setIsStaticCall(bool isStatic)
    {
        isStaticCall = isStatic;
    }

    void MethodCallNode::setGenericTypeArguments(const std::vector<std::string>& genericArgs)
    {
        genericTypeArguments = genericArgs;
    }

    void MethodCallNode::addArgument(std::unique_ptr<ASTNode> arg)
    {
        arguments.push_back(std::move(arg));
    }

    size_t MethodCallNode::getArgumentCount() const
    {
        return arguments.size();
    }

    Value MethodCallNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMethodCallNode(this);
    }

    std::unique_ptr<ASTNode> MethodCallNode::clone() const
    {
        std::unique_ptr<ASTNode> clonedObject = object ? object->clone() : nullptr;

        std::vector<std::unique_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(arg->clone());
            }
        }

        return std::make_unique<MethodCallNode>(
            std::move(clonedObject),
            methodName,
            std::move(clonedArgs),
            isStaticCall,
            genericTypeArguments,
            location
        );
    }
}
