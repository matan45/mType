#include "FunctionNode.hpp"
#include "../../utils/GenericTypeConversionUtils.hpp"

namespace ast::nodes::functions
{
    // NEW: Primary constructor with generic support
    FunctionNode::FunctionNode(const std::string& funcName,
                               std::shared_ptr<GenericType> retType,
                               const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params,
                               std::shared_ptr<ASTNode> funcBody,
                               const std::vector<GenericTypeParameter>& generics,
                               bool async,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), genericParameters(generics), returnType(retType),
          parameters(params), body(std::move(funcBody)), isAsync(async), visibility(VisibilityModifier::PUBLIC)
    {
    }

    // Backward compatibility constructor with ValueType (shared_ptr)
    FunctionNode::FunctionNode(const std::string& funcName, ValueType retType,
                               const std::vector<std::pair<std::string, ValueType>>& params,
                               std::shared_ptr<ASTNode> funcBody,
                               bool async,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), body(std::move(funcBody)), isAsync(async), visibility(VisibilityModifier::PUBLIC)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
    }

    // Backward compatibility constructor with ValueType (unique_ptr)
    FunctionNode::FunctionNode(const std::string& funcName, ValueType retType,
                               const std::vector<std::pair<std::string, ValueType>>& params,
                               std::unique_ptr<ASTNode> funcBody,
                               bool async,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), body(std::move(funcBody)), isAsync(async), visibility(VisibilityModifier::PUBLIC)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
    }

    const std::string& FunctionNode::getName() const noexcept
    {
        return name;
    }

    std::shared_ptr<GenericType> FunctionNode::getGenericReturnType() const noexcept
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& FunctionNode::getGenericParameters() const noexcept
    {
        return parameters;
    }

    // Legacy getter for backward compatibility
    ValueType FunctionNode::getReturnType() const noexcept
    {
        return utils::GenericTypeConversionUtils::convertGenericTypeToValueType(returnType);
    }

    // Legacy getter for backward compatibility - converts GenericType back to ValueType
    // Legacy getter with lazy cache for O(1) performance after first call
    std::vector<std::pair<std::string, ValueType>> FunctionNode::getParameters() const
    {
        if (!paramCacheValid)
        {
            cachedLegacyParams = utils::GenericTypeConversionUtils::convertParametersToValueType(parameters);
            paramCacheValid = true;
        }
        return *cachedLegacyParams;
    }

    std::vector<std::pair<std::string, ParameterType>> FunctionNode::getParameterTypes() const
    {
        std::vector<std::pair<std::string, ParameterType>> result;

        for (const auto& param : parameters) {
            result.emplace_back(param.first, classifyParameterType(param.second));
        }

        return result;
    }

    ParameterType FunctionNode::classifyParameterType(const std::shared_ptr<GenericType>& paramType) const
    {
        if (paramType->isGenericParameter()) {
            // Check if it's a real generic parameter (single letter like T, K, V)
            // or a class/interface name (like Vehicle, Animal, GenericContainer<Int>)
            std::string typeName = paramType->getGenericName();
            if (typeName.length() == 1 && std::isupper(typeName[0])) {
                // Real generic parameter - treat as plain object type
                return ParameterType(ValueType::OBJECT);
            } else {
                // Class or interface name - store as class type with full type info
                // Use toString() to preserve generic type arguments like GenericContainer<Int>
                return ParameterType::forClass(paramType->toString());
            }
        } else if (paramType->getConcreteType() == ValueType::OBJECT) {
            // Object type (class or interface) - store as class type with full type info
            // Use toString() to preserve generic type arguments
            return ParameterType::forClass(paramType->toString());
        } else {
            // Basic type (int, string, bool, etc.)
            return ParameterType(paramType->getConcreteType());
        }
    }

    std::shared_ptr<ASTNode> FunctionNode::getBody() const noexcept
    {
        return body;
    }

    ASTNode* FunctionNode::getBodyPtr() const noexcept
    {
        return body.get();
    }

    const std::vector<GenericTypeParameter>& FunctionNode::getGenericTypeParameters() const noexcept
    {
        return genericParameters;
    }

    void FunctionNode::setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics)
    {
        genericParameters = generics;
    }

    void FunctionNode::addGenericTypeParameter(const GenericTypeParameter& param)
    {
        genericParameters.push_back(param);
    }

    size_t FunctionNode::getGenericTypeParameterCount() const noexcept
    {
        return genericParameters.size();
    }

    void FunctionNode::setName(const std::string& funcName)
    {
        name = funcName;
    }

    void FunctionNode::setGenericReturnType(std::shared_ptr<GenericType> retType)
    {
        returnType = retType;
    }

    void FunctionNode::setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params)
    {
        parameters = params;
        paramCacheValid = false;  // Invalidate cache when parameters change
    }

    // Legacy setter for backward compatibility
    void FunctionNode::setReturnType(ValueType retType)
    {
        returnType = utils::GenericTypeConversionUtils::convertValueTypeToGenericType(retType);
    }

    // Legacy setter for backward compatibility
    void FunctionNode::setParameters(const std::vector<std::pair<std::string, ValueType>>& params)
    {
        parameters = utils::GenericTypeConversionUtils::convertParametersToGenericType(params);
        paramCacheValid = false;  // Invalidate cache when parameters change
    }

    void FunctionNode::setBody(std::shared_ptr<ASTNode> funcBody)
    {
        body = std::move(funcBody);
    }

    size_t FunctionNode::getParameterCount() const noexcept
    {
        return parameters.size();
    }

    VisibilityModifier FunctionNode::getVisibility() const noexcept
    {
        return visibility;
    }

    void FunctionNode::setVisibility(VisibilityModifier vis)
    {
        visibility = vis;
    }

    bool FunctionNode::isPublic() const noexcept
    {
        return visibility == VisibilityModifier::PUBLIC;
    }

    bool FunctionNode::isPrivate() const noexcept
    {
        return visibility == VisibilityModifier::PRIVATE;
    }

    const std::vector<std::shared_ptr<annotations::AnnotationNode>>& FunctionNode::getAnnotations() const
    {
        return annotations;
    }

    void FunctionNode::addAnnotation(std::shared_ptr<annotations::AnnotationNode> annotation)
    {
        annotations.push_back(annotation);
    }

    bool FunctionNode::hasAnnotation(const std::string& annotationName) const
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

    std::shared_ptr<annotations::AnnotationNode> FunctionNode::getAnnotation(const std::string& annotationName) const
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

    Value FunctionNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFunctionNode(this);
    }

    std::unique_ptr<ASTNode> FunctionNode::clone() const
    {
        // Clone the body (shared_ptr -> unique_ptr for constructor, then convert back to shared_ptr)
        std::shared_ptr<ASTNode> clonedBody = body ? std::shared_ptr<ASTNode>(body->clone()) : nullptr;

        // Clone generic parameters (copy constructor works for GenericTypeParameter)
        std::vector<GenericTypeParameter> clonedGenericParams = genericParameters;

        // Clone parameters and return type using utility functions
        auto clonedParams = utils::GenericTypeConversionUtils::cloneGenericParameters(parameters);
        auto clonedReturnType = utils::GenericTypeConversionUtils::cloneGenericType(returnType);

        auto clonedFunction = std::make_unique<FunctionNode>(
            name, clonedReturnType, clonedParams, clonedBody, clonedGenericParams, isAsync, location
        );

        clonedFunction->setVisibility(visibility);

        // Clone annotations
        for (const auto& annotation : annotations) {
            if (annotation) {
                // Safely reconstruct annotation using make_shared to avoid unsafe cast
                auto clonedAnnotation = std::make_shared<annotations::AnnotationNode>(
                    annotation->getName(),
                    annotation->getParameters(),
                    annotation->getLocation()
                );
                clonedFunction->addAnnotation(clonedAnnotation);
            }
        }

        return clonedFunction;
    }
}