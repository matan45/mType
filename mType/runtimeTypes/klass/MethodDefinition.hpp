#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>
#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../Definition.hpp"


namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class MethodDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::vector<std::pair<std::string, Value>> arguments;
        std::shared_ptr<ASTNode> body;
        bool isStaticMethod;

        // NEW: Generic type information for runtime type resolution
        std::shared_ptr<ast::GenericType> genericReturnType;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> genericParameters;
        std::vector<ast::GenericTypeParameter> genericTypeParameters;  // NEW: Store generic type parameter declarations (<T>, <K,V>)
        std::unordered_map<std::string, std::string> typeSubstitutionMap;  // For instantiated generic methods

    public:
        // Legacy constructor for backward compatibility
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s)
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStaticMethod(s),
              genericReturnType(nullptr), genericParameters(), typeSubstitutionMap()
        {
        }

        // NEW: Constructor with generic type information
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s,
                         std::shared_ptr<ast::GenericType> genRetType,
                         const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& genParams,
                         const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                         const std::unordered_map<std::string, std::string>& substitutions = {})
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStaticMethod(s),
              genericReturnType(genRetType), genericParameters(genParams), genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions)
        {
        }

        bool matchesArgCount(size_t argCount) const;
        
        const ValueType& getReturnType() const { return returnType; }
        void setReturnType(const ValueType& rt) { returnType = rt; }
        
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) { parameters = params; }
        
        ASTNode* getBody() const { return body.get(); }
        std::shared_ptr<ASTNode> getBodyPtr() const { return body; }
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }
        
        bool isStatic() const { return isStaticMethod; }
        void setStatic(bool s) { isStaticMethod = s; }

        // NEW: Generic type information getters and setters
        std::shared_ptr<ast::GenericType> getGenericReturnType() const { return genericReturnType; }
        void setGenericReturnType(std::shared_ptr<ast::GenericType> genRetType) { genericReturnType = genRetType; }

        const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& getGenericParameters() const { return genericParameters; }
        void setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& genParams) { genericParameters = genParams; }

        const std::vector<ast::GenericTypeParameter>& getGenericTypeParameters() const { return genericTypeParameters; }
        void setGenericTypeParameters(const std::vector<ast::GenericTypeParameter>& genTypeParams) { genericTypeParameters = genTypeParams; }

        const std::unordered_map<std::string, std::string>& getTypeSubstitutionMap() const { return typeSubstitutionMap; }
        void setTypeSubstitutionMap(const std::unordered_map<std::string, std::string>& substitutions) { typeSubstitutionMap = substitutions; }

        // NEW: Runtime type resolution methods
        ValueType resolveParameterType(size_t paramIndex) const;
        ValueType resolveParameterType(const std::string& paramName) const;
        ValueType resolveReturnType() const;
        bool hasGenericInformation() const { return genericReturnType != nullptr || !genericParameters.empty(); }
    };
}
