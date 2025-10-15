#include "FunctionNode.hpp"

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
        // Convert ValueType to GenericType
        returnType = std::make_shared<GenericType>(retType);

        // Convert parameters from ValueType to GenericType
        for (const auto& param : params) {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    // Backward compatibility constructor with ValueType (unique_ptr)
    FunctionNode::FunctionNode(const std::string& funcName, ValueType retType,
                               const std::vector<std::pair<std::string, ValueType>>& params,
                               std::unique_ptr<ASTNode> funcBody,
                               bool async,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), body(std::move(funcBody)), isAsync(async), visibility(VisibilityModifier::PUBLIC)
    {
        // Convert ValueType to GenericType
        returnType = std::make_shared<GenericType>(retType);

        // Convert parameters from ValueType to GenericType
        for (const auto& param : params) {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    const std::string& FunctionNode::getName() const
    {
        return name;
    }

    std::shared_ptr<GenericType> FunctionNode::getGenericReturnType() const
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& FunctionNode::getGenericParameters() const
    {
        return parameters;
    }

    // Legacy getter for backward compatibility
    ValueType FunctionNode::getReturnType() const
    {
        if (returnType && !returnType->isGenericParameter()) {
            return returnType->getConcreteType();
        }
        return ValueType::OBJECT; // Default for generic parameters
    }

    // Legacy getter for backward compatibility - converts GenericType back to ValueType
    const std::vector<std::pair<std::string, ValueType>>& FunctionNode::getParameters() const
    {
        // Similar to MethodNode, use static vector for compatibility
        static std::vector<std::pair<std::string, ValueType>> legacyParams;
        legacyParams.clear();

        for (const auto& param : parameters) {
            ValueType type = param.second->isGenericParameter() ?
                ValueType::OBJECT : param.second->getConcreteType();
            legacyParams.emplace_back(param.first, type);
        }

        return legacyParams;
    }

    std::vector<std::pair<std::string, ParameterType>> FunctionNode::getParameterTypes() const
    {
        std::vector<std::pair<std::string, ParameterType>> result;

        for (const auto& param : parameters) {
            if (param.second->isGenericParameter()) {
                // Check if it's a real generic parameter (single letter like T, K, V)
                // or a class/interface name (like Vehicle, Animal, GenericContainer<Int>)
                std::string typeName = param.second->getGenericName();
                if (typeName.length() == 1 && std::isupper(typeName[0])) {
                    // Real generic parameter - treat as plain object type
                    result.emplace_back(param.first, ParameterType(ValueType::OBJECT));
                } else {
                    // Class or interface name - store as class type with full type info
                    // Use toString() to preserve generic type arguments like GenericContainer<Int>
                    result.emplace_back(param.first, ParameterType::forClass(param.second->toString()));
                }
            } else if (param.second->getConcreteType() == ValueType::OBJECT) {
                // Object type (class or interface) - store as class type with full type info
                // Use toString() to preserve generic type arguments
                result.emplace_back(param.first, ParameterType::forClass(param.second->toString()));
            } else {
                // Basic type (int, string, bool, etc.)
                result.emplace_back(param.first, ParameterType(param.second->getConcreteType()));
            }
        }

        return result;
    }

    std::shared_ptr<ASTNode> FunctionNode::getBody() const
    {
        return body;
    }

    ASTNode* FunctionNode::getBodyPtr() const
    {
        return body.get();
    }

    const std::vector<GenericTypeParameter>& FunctionNode::getGenericTypeParameters() const
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

    size_t FunctionNode::getGenericTypeParameterCount() const
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
    }

    // Legacy setter for backward compatibility
    void FunctionNode::setReturnType(ValueType retType)
    {
        returnType = std::make_shared<GenericType>(retType);
    }

    // Legacy setter for backward compatibility
    void FunctionNode::setParameters(const std::vector<std::pair<std::string, ValueType>>& params)
    {
        parameters.clear();
        for (const auto& param : params) {
            parameters.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }
    }

    void FunctionNode::setBody(std::shared_ptr<ASTNode> funcBody)
    {
        body = std::move(funcBody);
    }

    size_t FunctionNode::getParameterCount() const
    {
        return parameters.size();
    }

    VisibilityModifier FunctionNode::getVisibility() const
    {
        return visibility;
    }

    void FunctionNode::setVisibility(VisibilityModifier vis)
    {
        visibility = vis;
    }

    bool FunctionNode::isPublic() const
    {
        return visibility == VisibilityModifier::PUBLIC;
    }

    bool FunctionNode::isPrivate() const
    {
        return visibility == VisibilityModifier::PRIVATE;
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

        // Clone parameters with GenericType (need deep copy of shared_ptrs)
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> clonedParams;
        clonedParams.reserve(parameters.size());
        for (const auto& param : parameters) {
            // Deep copy GenericType via copy constructor
            auto clonedGenericType = std::make_shared<GenericType>(*param.second);
            clonedParams.emplace_back(param.first, clonedGenericType);
        }

        // Clone return type
        std::shared_ptr<GenericType> clonedReturnType =
            returnType ? std::make_shared<GenericType>(*returnType) : nullptr;

        auto clonedFunction = std::make_unique<FunctionNode>(
            name, clonedReturnType, clonedParams, clonedBody, clonedGenericParams, isAsync, location
        );

        clonedFunction->setVisibility(visibility);

        return clonedFunction;
    }
}