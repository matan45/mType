#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../Definition.hpp"

// Forward declarations to avoid circular dependency
namespace value {
    class LambdaValue;
}

namespace ast::nodes::expressions {
    class LambdaNode;
}


namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class MethodDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ParameterType>> parameters;
        std::vector<std::pair<std::string, Value>> arguments;
        std::shared_ptr<ASTNode> body;
        bool isStaticMethod;

        // Lambda implementation storage for interface methods
        std::shared_ptr<value::LambdaValue> lambdaImplementation;

        // Store lambda node for deferred LambdaValue creation with smart pointer for memory safety
        std::weak_ptr<ast::nodes::expressions::LambdaNode> lambdaNode;

        // NEW: Generic type information for runtime type resolution
        std::shared_ptr<ast::GenericType> genericReturnType;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> genericParameters;
        std::vector<ast::GenericTypeParameter> genericTypeParameters;  // NEW: Store generic type parameter declarations (<T>, <K,V>)
        std::unordered_map<std::string, std::string> typeSubstitutionMap;  // For instantiated generic methods

    public:
        // Legacy constructor for backward compatibility with ValueType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s)
            : Definition(n), returnType(rt), parameters(ParameterType::fromValueTypeVector(params)), arguments(args), body(b), isStaticMethod(s),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(nullptr), genericParameters(), typeSubstitutionMap()
        {
        }

        // New constructor with ParameterType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ParameterType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s)
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStaticMethod(s),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(nullptr), genericParameters(), typeSubstitutionMap()
        {
        }

        // NEW: Constructor with generic type information (ValueType legacy)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s,
                         std::shared_ptr<ast::GenericType> genRetType,
                         const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& genParams,
                         const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                         const std::unordered_map<std::string, std::string>& substitutions = {})
            : Definition(n), returnType(rt), parameters(ParameterType::fromValueTypeVector(params)), arguments(args), body(b), isStaticMethod(s),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(genRetType), genericParameters(genParams), genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions)
        {
        }

        // NEW: Constructor with generic type information (ParameterType)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ParameterType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s,
                         std::shared_ptr<ast::GenericType> genRetType,
                         const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& genParams,
                         const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                         const std::unordered_map<std::string, std::string>& substitutions = {})
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStaticMethod(s),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(genRetType), genericParameters(genParams), genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions)
        {
        }

        bool matchesArgCount(size_t argCount) const;
        
        const ValueType& getReturnType() const { return returnType; }
        void setReturnType(const ValueType& rt) { returnType = rt; }
        
        const std::vector<std::pair<std::string, ParameterType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ParameterType>>& params) { parameters = params; }

        // Backward compatibility methods for ValueType
        std::vector<std::pair<std::string, ValueType>> getParametersAsValueType() const {
            return ParameterType::toValueTypeVector(parameters);
        }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) {
            parameters = ParameterType::fromValueTypeVector(params);
        }
        
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

        // Lambda implementation methods
        std::shared_ptr<value::LambdaValue> getLambdaImplementation() const { return lambdaImplementation; }
        void setLambdaImplementation(std::shared_ptr<value::LambdaValue> lambda) { lambdaImplementation = lambda; }
        bool hasLambdaImplementation() const { return lambdaImplementation != nullptr; }

        // Lambda node storage for deferred LambdaValue creation
        // Memory-safe lambda node access using weak_ptr to prevent dangling references
        std::shared_ptr<ast::nodes::expressions::LambdaNode> getLambdaNode() const {
            return lambdaNode.lock(); // Returns shared_ptr or nullptr if expired
        }
        void setLambdaNode(std::shared_ptr<ast::nodes::expressions::LambdaNode> node) {
            lambdaNode = node; // Store as weak_ptr to avoid circular references
        }
        bool hasLambdaNode() const { return !lambdaNode.expired(); }

        // Memory safety check for lambda node validity
        bool isLambdaNodeValid() const {
            return !lambdaNode.expired(); // Check if weak_ptr is still valid
        }

        // Enhanced memory safety and cleanup
        void cleanupLambdaResources();
        bool needsLambdaCleanup() const;
        std::string getLambdaLifecycleStatus() const;

        // NEW: Runtime type resolution methods
        ValueType resolveParameterType(size_t paramIndex) const;
        ValueType resolveParameterType(const std::string& paramName) const;
        ValueType resolveReturnType() const;
        bool hasGenericInformation() const { return genericReturnType != nullptr || !genericParameters.empty(); }
    };
}
