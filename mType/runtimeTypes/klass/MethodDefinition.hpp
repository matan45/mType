#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../ast/GenericType.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../Definition.hpp"

// Forward declarations to avoid circular dependency
namespace value
{
    class LambdaValue;
}

namespace ast::nodes::expressions
{
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
        std::shared_ptr<ASTNode> body;
        bool isStaticMethod;
        ast::AccessModifier accessModifier;

        // Lambda implementation storage for interface methods
        std::shared_ptr<value::LambdaValue> lambdaImplementation;

        // Store lambda node for deferred LambdaValue creation with smart pointer for memory safety
        std::weak_ptr<ast::nodes::expressions::LambdaNode> lambdaNode;

        // NEW: Generic type information for runtime type resolution
        std::shared_ptr<ast::GenericType> genericReturnType;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> genericParameters;
        std::vector<ast::GenericTypeParameter> genericTypeParameters;
        // NEW: Store generic type parameter declarations (<T>, <K,V>)
        std::unordered_map<std::string, std::string> typeSubstitutionMap; // For instantiated generic methods

        bool isAsync;  // NEW: Flag to indicate async method
        bool abstractMethod;  // NEW: Flag to indicate abstract method

        // NEW: Annotations for this method
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;

        // Helper methods for resolveParameterType
        ValueType resolveGenericParameter(size_t paramIndex, ValueType storedType) const;
        ValueType resolveFallbackMapping(size_t paramIndex) const;

        // Validation helper methods
        void validateGenericInvariants() const;
        void validateParameterCounts(size_t paramCount, size_t genParamCount) const;
        void validateSubstitutionMap() const;
        void validateGenericTypeRecursive(const std::shared_ptr<ast::GenericType>& type, const std::string& context) const;

    public:
        // Legacy constructor for backward compatibility with ValueType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ValueType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(ParameterTypeConverter::fromValueTypeVector(params)),
              body(b), isStaticMethod(s), accessModifier(modifier),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(nullptr), genericParameters(),
              typeSubstitutionMap(), isAsync(false), abstractMethod(false)
        {
        }

        // New constructor with ParameterType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ParameterType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(params), body(b), isStaticMethod(s),
              accessModifier(modifier), lambdaImplementation(nullptr), lambdaNode(), genericReturnType(nullptr),
              genericParameters(), typeSubstitutionMap(), isAsync(false), abstractMethod(false)
        {
        }

        // NEW: Constructor with generic type information (ValueType legacy)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ValueType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  std::shared_ptr<ast::GenericType> genRetType,
                                  const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>&
                                  genParams,
                                  const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                                  const std::unordered_map<std::string, std::string>& substitutions = {},
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(ParameterTypeConverter::fromValueTypeVector(params)),
              body(b), isStaticMethod(s), accessModifier(modifier),
              lambdaImplementation(nullptr), lambdaNode(), genericReturnType(genRetType), genericParameters(genParams),
              genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions), isAsync(false), abstractMethod(false)
        {
            validateParameterCounts(params.size(), genParams.size());
            validateGenericInvariants();
        }

        // NEW: Constructor with generic type information (ParameterType)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ParameterType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  std::shared_ptr<ast::GenericType> genRetType,
                                  const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>&
                                  genParams,
                                  const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                                  const std::unordered_map<std::string, std::string>& substitutions = {},
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(params), body(b), isStaticMethod(s),
              accessModifier(modifier), lambdaImplementation(nullptr), lambdaNode(), genericReturnType(genRetType),
              genericParameters(genParams), genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions),
              isAsync(false), abstractMethod(false)
        {
            validateParameterCounts(params.size(), genParams.size());
            validateGenericInvariants();
        }

        const ValueType& getReturnType() const { return returnType; }
        void setReturnType(const ValueType& rt) { returnType = rt; }

        const std::vector<std::pair<std::string, ParameterType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ParameterType>>& params) { parameters = params; }

        // Backward compatibility methods for ValueType
        std::vector<std::pair<std::string, ValueType>> getParametersAsValueType() const
        {
            return ParameterTypeConverter::toValueTypeVector(parameters);
        }

        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params)
        {
            parameters = ParameterTypeConverter::fromValueTypeVector(params);
        }

        ASTNode* getBody() const { return body.get(); }
        std::shared_ptr<ASTNode> getBodyPtr() const { return body; }
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }

        bool isStatic() const { return isStaticMethod; }
        void setStatic(bool s) { isStaticMethod = s; }

        ast::AccessModifier getAccessModifier() const { return accessModifier; }
        void setAccessModifier(ast::AccessModifier modifier) { accessModifier = modifier; }

        // NEW: Generic type information getters and setters
        std::shared_ptr<ast::GenericType> getGenericReturnType() const { return genericReturnType; }
        void setGenericReturnType(std::shared_ptr<ast::GenericType> genRetType) { genericReturnType = genRetType; }

        const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& getGenericParameters() const
        {
            return genericParameters;
        }

        void setGenericParameters(
            const std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>& genParams)
        {
            genericParameters = genParams;
        }

        const std::vector<ast::GenericTypeParameter>& getGenericTypeParameters() const { return genericTypeParameters; }

        void setGenericTypeParameters(const std::vector<ast::GenericTypeParameter>& genTypeParams)
        {
            genericTypeParameters = genTypeParams;
        }

        const std::unordered_map<std::string, std::string>& getTypeSubstitutionMap() const
        {
            return typeSubstitutionMap;
        }

        void setTypeSubstitutionMap(const std::unordered_map<std::string, std::string>& substitutions)
        {
            typeSubstitutionMap = substitutions;
        }

        // Lambda implementation methods
        std::shared_ptr<value::LambdaValue> getLambdaImplementation() const { return lambdaImplementation; }
        void setLambdaImplementation(std::shared_ptr<value::LambdaValue> lambda) { lambdaImplementation = lambda; }
        bool hasLambdaImplementation() const { return lambdaImplementation != nullptr; }

        // Lambda node storage for deferred LambdaValue creation
        // Memory-safe lambda node access using weak_ptr to prevent dangling references
        std::shared_ptr<ast::nodes::expressions::LambdaNode> getLambdaNode() const
        {
            return lambdaNode.lock(); // Returns shared_ptr or nullptr if expired
        }

        void setLambdaNode(std::shared_ptr<ast::nodes::expressions::LambdaNode> node)
        {
            lambdaNode = node; // Store as weak_ptr to avoid circular references
        }

        bool hasLambdaNode() const { return !lambdaNode.expired(); }

        // Memory safety check for lambda node validity
        bool isLambdaNodeValid() const
        {
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
        bool isGeneric() const { return genericReturnType != nullptr || !genericParameters.empty(); }

        // NEW: Async support
        bool getIsAsync() const { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Abstract support
        bool isAbstract() const { return abstractMethod; }
        void setAbstract(bool isAbstract) { abstractMethod = isAbstract; }

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& getAnnotations() const { return annotations; }
        void addAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation) { annotations.push_back(annotation); }
        bool hasAnnotation(const std::string& annotationName) const;
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const;
    };
}
