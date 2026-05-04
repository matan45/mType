#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../types/UnifiedType.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../errors/SourceLocation.hpp"
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

        // Unified type information for runtime type resolution
        ::types::UnifiedTypePtr unifiedReturnType;
        std::vector<std::pair<std::string, ::types::UnifiedTypePtr>> unifiedParameters;
        std::vector<ast::GenericTypeParameter> genericTypeParameters;
        // Store generic type parameter declarations (<T>, <K,V>)
        std::unordered_map<std::string, std::string> typeSubstitutionMap; // For instantiated generic methods

        bool isAsync;  // NEW: Flag to indicate async method
        bool abstractMethod;  // NEW: Flag to indicate abstract method
        bool finalMethod;  // NEW: Flag to indicate final method (cannot be overridden)
        bool synthetic = false;  // MYT-274: compiler-synthesized method (e.g. auto hashCode/equals)

        // NEW: Annotations for this method
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;

        // MYT-110: per-parameter annotations, parallel to `parameters`.
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;

        // NEW: Source location for error reporting
        ast::SourceLocation sourceLocation;

        // Helper methods for resolveParameterType
        ValueType resolveGenericParameter(size_t paramIndex, ValueType storedType) const;
        ValueType resolveFallbackMapping(size_t paramIndex) const;

        // Validation helper methods
        void validateGenericInvariants() const;
        void validateParameterCounts(size_t paramCount, size_t genParamCount) const;
        void validateSubstitutionMap() const;
        void validateGenericTypeRecursive(const ::types::UnifiedTypePtr& type, const std::string& context) const;

    public:
        // Legacy constructor for backward compatibility with ValueType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ValueType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(ParameterTypeConverter::fromValueTypeVector(params)),
              body(b), isStaticMethod(s), accessModifier(modifier),
              lambdaImplementation(nullptr), lambdaNode(), unifiedReturnType(nullptr), unifiedParameters(),
              typeSubstitutionMap(), isAsync(false), abstractMethod(false), finalMethod(false)
        {
        }

        // New constructor with ParameterType
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ParameterType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(params), body(b), isStaticMethod(s),
              accessModifier(modifier), lambdaImplementation(nullptr), lambdaNode(), unifiedReturnType(nullptr),
              unifiedParameters(), typeSubstitutionMap(), isAsync(false), abstractMethod(false), finalMethod(false)
        {
        }

        // Constructor with unified type information (ValueType legacy)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ValueType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ::types::UnifiedTypePtr uRetType,
                                  const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>&
                                  uParams,
                                  const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                                  const std::unordered_map<std::string, std::string>& substitutions = {},
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(ParameterTypeConverter::fromValueTypeVector(params)),
              body(b), isStaticMethod(s), accessModifier(modifier),
              lambdaImplementation(nullptr), lambdaNode(), unifiedReturnType(std::move(uRetType)), unifiedParameters(uParams),
              genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions), isAsync(false), abstractMethod(false), finalMethod(false)
        {
            validateParameterCounts(params.size(), uParams.size());
            validateGenericInvariants();
        }

        // Constructor with unified type information (ParameterType)
        explicit MethodDefinition(const std::string& n, ValueType rt,
                                  const std::vector<std::pair<std::string, ParameterType>>& params,
                                  std::shared_ptr<ASTNode> b, bool s,
                                  ::types::UnifiedTypePtr uRetType,
                                  const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>&
                                  uParams,
                                  const std::vector<ast::GenericTypeParameter>& genTypeParams = {},
                                  const std::unordered_map<std::string, std::string>& substitutions = {},
                                  ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), returnType(rt), parameters(params), body(b), isStaticMethod(s),
              accessModifier(modifier), lambdaImplementation(nullptr), lambdaNode(), unifiedReturnType(std::move(uRetType)),
              unifiedParameters(uParams), genericTypeParameters(genTypeParams), typeSubstitutionMap(substitutions),
              isAsync(false), abstractMethod(false), finalMethod(false)
        {
            validateParameterCounts(params.size(), uParams.size());
            validateGenericInvariants();
        }

        const ValueType& getReturnType() const { return returnType; }
        void setReturnType(const ValueType& rt) { returnType = rt; }

        const std::vector<std::pair<std::string, ParameterType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ParameterType>>& params) { parameters = params; }

        // NEW: Alias for clarity in overload resolution code
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const { return parameters; }

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

        // Unified type information getters and setters
        ::types::UnifiedTypePtr getUnifiedReturnType() const { return unifiedReturnType; }
        void setUnifiedReturnType(::types::UnifiedTypePtr type) { unifiedReturnType = std::move(type); }

        const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>& getUnifiedParameters() const
        {
            return unifiedParameters;
        }

        void setUnifiedParameters(
            const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>& uParams)
        {
            unifiedParameters = uParams;
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
        bool isGeneric() const { return unifiedReturnType != nullptr || !unifiedParameters.empty(); }

        // NEW: Async support
        bool getIsAsync() const { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Abstract support
        bool isAbstract() const { return abstractMethod; }
        void setAbstract(bool isAbstract) { abstractMethod = isAbstract; }

        // NEW: Final support
        bool isFinal() const { return finalMethod; }
        void setFinal(bool isFinalMethod) { finalMethod = isFinalMethod; }

        // MYT-274: synthetic flag for compiler-generated methods
        bool isSynthetic() const { return synthetic; }
        void setSynthetic(bool s) { synthetic = s; }

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& getAnnotations() const { return annotations; }
        void addAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation) { annotations.push_back(annotation); }
        bool hasAnnotation(const std::string& annotationName) const;
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const;

        // MYT-110: per-parameter annotations
        const std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>&
        getParameterAnnotations() const { return parameterAnnotations; }

        void setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> annotationsByIndex)
        {
            parameterAnnotations = std::move(annotationsByIndex);
            parameterAnnotations.resize(parameters.size());
        }

        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>&
        getParameterAnnotations(size_t paramIndex) const
        {
            static const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> empty;
            if (paramIndex >= parameterAnnotations.size()) return empty;
            return parameterAnnotations[paramIndex];
        }

        bool hasParameterAnnotation(size_t paramIndex, const std::string& name) const
        {
            for (const auto& a : getParameterAnnotations(paramIndex))
                if (a && a->getName() == name) return true;
            return false;
        }

        std::shared_ptr<ast::nodes::annotations::AnnotationNode>
        getParameterAnnotation(size_t paramIndex, const std::string& name) const
        {
            for (const auto& a : getParameterAnnotations(paramIndex))
                if (a && a->getName() == name) return a;
            return nullptr;
        }

        // NEW: Source location methods
        const ast::SourceLocation& getSourceLocation() const { return sourceLocation; }
        void setSourceLocation(const ast::SourceLocation& location) { sourceLocation = location; }

        // Reflection support: get modifier flags as bitmask
        // PUBLIC=1, PRIVATE=2, PROTECTED=4, STATIC=8, FINAL=16, ABSTRACT=32, ASYNC=64, SYNTHETIC=128
        int getModifierFlags() const {
            int flags = 0;
            switch (accessModifier) {
                case ast::AccessModifier::PUBLIC: flags |= 1; break;
                case ast::AccessModifier::PRIVATE: flags |= 2; break;
                case ast::AccessModifier::PROTECTED: flags |= 4; break;
            }
            if (isStaticMethod) flags |= 8;
            if (finalMethod) flags |= 16;
            if (abstractMethod) flags |= 32;
            if (isAsync) flags |= 64;
            if (synthetic) flags |= 128;
            return flags;
        }

        // Get type signature for bytecode function lookup (e.g., "float" for onUpdate(float deltaTime))
        std::string getTypeSignature() const;
    };
}
