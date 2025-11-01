#include "MethodNode.hpp"
#include "../../utils/GenericTypeConversionUtils.hpp"

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
                           bool async,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), genericParameters(generics), returnType(retType),
          parameters(params), body(std::move(methodBody)), isStatic(isStaticMethod), isAsync(async),
          abstractMethod(false), finalMethod(false), accessModifier(modifier)
    {
    }

    // Backward compatibility constructor with ValueType (shared_ptr)
    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           const std::vector<std::pair<std::string, ValueType>>& params,
                           std::shared_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           AccessModifier modifier,
                           bool async,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), isStatic(isStaticMethod), isAsync(async), abstractMethod(false),
          finalMethod(false), body(std::move(methodBody)), accessModifier(modifier)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
    }

    // Backward compatibility constructor with ValueType (unique_ptr)
    MethodNode::MethodNode(const std::string& methodName, ValueType retType,
                           const std::vector<std::pair<std::string, ValueType>>& params,
                           std::unique_ptr<ASTNode> methodBody,
                           bool isStaticMethod,
                           AccessModifier modifier,
                           bool async,
                           const SourceLocation& loc)
        : ASTNode(loc), name(methodName), isStatic(isStaticMethod), isAsync(async), abstractMethod(false),
          finalMethod(false), body(std::move(methodBody)), accessModifier(modifier)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
    }

    const std::string& MethodNode::getName() const noexcept
    {
        return name;
    }

    std::shared_ptr<GenericType> MethodNode::getGenericReturnType() const noexcept
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>&
    MethodNode::getGenericParameters() const noexcept
    {
        return parameters;
    }

    // Legacy getter for backward compatibility
    ValueType MethodNode::getReturnType() const noexcept
    {
        return utils::GenericTypeConversionUtils::convertGenericTypeToValueType(returnType);
    }

    // Legacy getter for backward compatibility - converts GenericType back to ValueType
    // Uses lazy cache for O(1) performance after first call
    std::vector<std::pair<std::string, ValueType>> MethodNode::getParameters() const
    {
        if (!paramCacheValid)
        {
            cachedLegacyParams = utils::GenericTypeConversionUtils::convertParametersToValueType(parameters);
            paramCacheValid = true;
        }
        return *cachedLegacyParams;
    }

    std::shared_ptr<ASTNode> MethodNode::getBody() const noexcept
    {
        return body;
    }

    ASTNode* MethodNode::getBodyPtr() const noexcept
    {
        return body.get();
    }

    bool MethodNode::getIsStatic() const noexcept
    {
        return isStatic;
    }

    const std::vector<GenericTypeParameter>& MethodNode::getGenericTypeParameters() const noexcept
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

    size_t MethodNode::getGenericTypeParameterCount() const noexcept
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
        paramCacheValid = false; // Invalidate cache when parameters change
    }

    // Legacy setter for backward compatibility
    void MethodNode::setReturnType(ValueType retType)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
    }

    // Legacy setter for backward compatibility
    void MethodNode::setParameters(const std::vector<std::pair<std::string, ValueType>>& params)
    {
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
        paramCacheValid = false; // Invalidate cache when parameters change
    }

    void MethodNode::setBody(std::shared_ptr<ASTNode> methodBody)
    {
        body = std::move(methodBody);
    }

    void MethodNode::setIsStatic(bool isStaticMethod)
    {
        isStatic = isStaticMethod;
    }

    AccessModifier MethodNode::getAccessModifier() const noexcept
    {
        return accessModifier;
    }

    void MethodNode::setAccessModifier(AccessModifier modifier)
    {
        accessModifier = modifier;
    }

    size_t MethodNode::getParameterCount() const noexcept
    {
        return parameters.size();
    }

    const std::vector<std::shared_ptr<annotations::AnnotationNode>>& MethodNode::getAnnotations() const
    {
        return annotations;
    }

    void MethodNode::addAnnotation(std::shared_ptr<annotations::AnnotationNode> annotation)
    {
        annotations.push_back(annotation);
    }

    bool MethodNode::hasAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<annotations::AnnotationNode> MethodNode::getAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return annotation;
            }
        }
        return nullptr;
    }

    Value MethodNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMethodNode(this);
    }

    std::unique_ptr<ASTNode> MethodNode::clone() const
    {
        // Clone the body (shared_ptr -> unique_ptr for constructor, then convert back to shared_ptr)
        std::shared_ptr<ASTNode> clonedBody = body ? std::shared_ptr<ASTNode>(body->clone()) : nullptr;

        // Clone generic parameters (copy constructor works for GenericTypeParameter)
        std::vector<GenericTypeParameter> clonedGenericParams = genericParameters;

        // Clone parameters and return type using utility functions
        auto clonedParams = utils::GenericTypeConversionUtils::cloneGenericParameters(parameters);
        auto clonedReturnType = utils::GenericTypeConversionUtils::cloneGenericType(returnType);

        auto cloned = std::make_unique<MethodNode>(
            name, clonedReturnType, clonedParams, clonedBody, isStatic,
            clonedGenericParams, accessModifier, isAsync, location
        );
        cloned->setAbstract(abstractMethod);
        cloned->setFinal(finalMethod);

        // Clone annotations
        for (const auto& annotation : annotations)
        {
            if (annotation)
            {
                // Safely reconstruct annotation using make_shared to avoid unsafe cast
                auto clonedAnnotation = std::make_shared<annotations::AnnotationNode>(
                    annotation->getName(),
                    annotation->getParameters(),
                    annotation->getLocation()
                );
                cloned->addAnnotation(clonedAnnotation);
            }
        }

        return cloned;
    }
}
