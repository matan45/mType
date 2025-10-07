#include "MethodNode.hpp"

namespace ast::nodes::classes
{
    // NEW: Primary constructor with generic support
    MethodNode::MethodNode(const std::string& methodName,
                           std::shared_ptr<GenericType> retType,
                           const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params,
                           std::shared_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           const std::vector<GenericTypeParameter>& generics,
                           AccessModifier modifier,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), genericParameters(generics), returnType(retType),
          parameters(params), body(std::move(methodBody)), isStatic(isStaticMethod), accessModifier(modifier)
    {
    }

    // Backward compatibility constructor with ValueType (shared_ptr)
    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           const std::vector<std::pair<std::string, ValueType>>& params,
                           std::shared_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           AccessModifier modifier,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), isStatic(isStaticMethod), body(std::move(methodBody)), accessModifier(modifier)
    {
        // Convert ValueType to GenericType
        returnType = std::make_shared<GenericType>(retType);

        // Convert parameters from ValueType to GenericType
        for (const auto& param : params)
        {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    // Backward compatibility constructor with ValueType (unique_ptr)
    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           const std::vector<std::pair<std::string, ValueType>>& params,
                           std::unique_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           AccessModifier modifier,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), isStatic(isStaticMethod), body(std::move(methodBody)), accessModifier(modifier)
    {
        // Convert ValueType to GenericType
        returnType = std::make_shared<GenericType>(retType);

        // Convert parameters from ValueType to GenericType
        for (const auto& param : params)
        {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    const std::string& MethodNode::getName() const
    {
        return name;
    }

    std::shared_ptr<GenericType> MethodNode::getGenericReturnType() const
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& MethodNode::getGenericParameters() const
    {
        return parameters;
    }

    // Legacy getter for backward compatibility
    ValueType MethodNode::getReturnType() const
    {
        if (returnType && !returnType->isGenericParameter())
        {
            return returnType->getConcreteType();
        }
        return ValueType::OBJECT; // Default for generic parameters
    }

    // Legacy getter for backward compatibility - converts GenericType back to ValueType
    const std::vector<std::pair<std::string, ValueType>>& MethodNode::getParameters() const
    {
        // This is a bit tricky since we need to return a reference
        // For now, we'll maintain a separate legacy parameters vector
        // In practice, code should migrate to using getGenericParameters()
        static std::vector<std::pair<std::string, ValueType>> legacyParams;
        legacyParams.clear();

        for (const auto& param : parameters)
        {
            ValueType type = param.second->isGenericParameter() ? ValueType::OBJECT : param.second->getConcreteType();
            legacyParams.emplace_back(param.first, type);
        }

        return legacyParams;
    }

    std::shared_ptr<ASTNode> MethodNode::getBody() const
    {
        return body;
    }

    ASTNode* MethodNode::getBodyPtr() const
    {
        return body.get();
    }

    bool MethodNode::getIsStatic() const
    {
        return isStatic;
    }

    const std::vector<GenericTypeParameter>& MethodNode::getGenericTypeParameters() const
    {
        return genericParameters;
    }

    void MethodNode::setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics)
    {
        genericParameters = generics;
    }

    void MethodNode::addGenericTypeParameter(const GenericTypeParameter& param)
    {
        genericParameters.push_back(param);
    }

    size_t MethodNode::getGenericTypeParameterCount() const
    {
        return genericParameters.size();
    }

    void MethodNode::setName(const std::string& methodName)
    {
        name = methodName;
    }

    void MethodNode::setGenericReturnType(std::shared_ptr<GenericType> retType)
    {
        returnType = retType;
    }

    void MethodNode::setGenericParameters(
        const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params)
    {
        parameters = params;
    }

    // Legacy setter for backward compatibility
    void MethodNode::setReturnType(ValueType retType)
    {
        returnType = std::make_shared<GenericType>(retType);
    }

    // Legacy setter for backward compatibility
    void MethodNode::setParameters(const std::vector<std::pair<std::string, ValueType>>& params)
    {
        parameters.clear();
        for (const auto& param : params)
        {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    void MethodNode::setBody(std::shared_ptr<ASTNode> methodBody)
    {
        body = std::move(methodBody);
    }

    void MethodNode::setIsStatic(bool isStaticMethod)
    {
        isStatic = isStaticMethod;
    }

    AccessModifier MethodNode::getAccessModifier() const
    {
        return accessModifier;
    }

    void MethodNode::setAccessModifier(AccessModifier modifier)
    {
        accessModifier = modifier;
    }

    size_t MethodNode::getParameterCount() const
    {
        return parameters.size();
    }

    Value MethodNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMethodNode(this);
    }
}
