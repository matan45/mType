#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include "../../GenericType.hpp"
#include "../../AccessModifier.hpp"
#include "../../../value/ValueType.hpp"
#include "../annotations/AnnotationNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace ast::nodes::classes
{
    using namespace value;

    class MethodNode : public ASTNode
    {
    private:
        std::string name;
        std::vector<GenericTypeParameter> genericParameters;  // NEW: For generic methods
        std::shared_ptr<GenericType> returnType;              // CHANGED: From ValueType to GenericType
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> parameters;  // CHANGED: GenericType instead of ValueType
        std::shared_ptr<ASTNode> body;
        bool isStatic;
        bool isAsync;  // NEW: Flag to indicate async method
        bool abstractMethod;  // NEW: Flag to indicate abstract method
        bool finalMethod;  // NEW: Flag to indicate final method (cannot be overridden)
        bool synthetic = false;  // MYT-274: compiler-synthesized method (e.g. auto-generated hashCode/equals)
        AccessModifier accessModifier;
        std::vector<std::shared_ptr<annotations::AnnotationNode>> annotations;  // NEW: Annotations for this method

        // MYT-110: per-parameter annotations, indexed by parameter position.
        // parameterAnnotations[i] holds the annotations on parameters[i].
        std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>> parameterAnnotations;

        // Performance cache for legacy getParameters() - O(1) after first call
        mutable std::optional<std::vector<std::pair<std::string, ValueType>>> cachedLegacyParams;
        mutable bool paramCacheValid = false;

    public:
        // NEW: Updated constructors with generic support
        explicit MethodNode(const std::string& methodName,
                           std::shared_ptr<GenericType> retType,
                           const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params,
                           std::shared_ptr<ASTNode> methodBody,
                           bool isStaticMethod = false,
                           const std::vector<GenericTypeParameter>& generics = {},
                           AccessModifier modifier = AccessModifier::PRIVATE,
                           bool async = false,
                           const SourceLocation& loc = SourceLocation());

        // Backward compatibility constructor with ValueType
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   const std::vector<std::pair<std::string, ValueType>>& params,
                   std::shared_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   AccessModifier modifier = AccessModifier::PRIVATE,
                   bool async = false,
                   const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   const std::vector<std::pair<std::string, ValueType>>& params,
                   std::unique_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   AccessModifier modifier = AccessModifier::PRIVATE,
                   bool async = false,
                   const SourceLocation& loc = SourceLocation());

        [[nodiscard]] const std::string& getName() const noexcept;

        // NEW: Generic-aware getters
        [[nodiscard]] std::shared_ptr<GenericType> getGenericReturnType() const noexcept;
        [[nodiscard]] const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& getGenericParameters() const noexcept;

        // Legacy getters for backward compatibility
        [[nodiscard]] ValueType getReturnType() const noexcept;
        [[deprecated("Use getGenericParameters() instead")]]
        [[nodiscard]] std::vector<std::pair<std::string, ValueType>> getParameters() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const noexcept;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;

        [[nodiscard]] bool getIsStatic() const noexcept;

        // NEW: Async-related methods
        [[nodiscard]] bool getIsAsync() const noexcept { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Abstract-related methods
        [[nodiscard]] bool isAbstract() const noexcept { return abstractMethod; }
        void setAbstract(bool isAbstract) { abstractMethod = isAbstract; }

        // NEW: Final-related methods
        [[nodiscard]] bool isFinal() const noexcept { return finalMethod; }
        void setFinal(bool isFinalMethod) { finalMethod = isFinalMethod; }

        // MYT-274: synthetic flag for compiler-generated methods
        [[nodiscard]] bool isSynthetic() const noexcept { return synthetic; }
        void setSynthetic(bool s) noexcept { synthetic = s; }

        // NEW: Generic-related methods
        [[nodiscard]] const std::vector<GenericTypeParameter>& getGenericTypeParameters() const noexcept;
        void setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics);
        void addGenericTypeParameter(const GenericTypeParameter& param);
        [[nodiscard]] size_t getGenericTypeParameterCount() const noexcept;
        [[nodiscard]] bool isGeneric() const noexcept { return !genericParameters.empty(); }

        void setName(const std::string& methodName);
        void setGenericReturnType(std::shared_ptr<GenericType> retType);
        void setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);

        // Legacy setters for backward compatibility
        void setReturnType(ValueType retType);
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params);

        void setBody(std::shared_ptr<ASTNode> methodBody);
        void setIsStatic(bool isStaticMethod);

        [[nodiscard]] AccessModifier getAccessModifier() const noexcept;
        void setAccessModifier(AccessModifier modifier);

        [[nodiscard]] size_t getParameterCount() const noexcept;

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<annotations::AnnotationNode>>& getAnnotations() const;
        void addAnnotation(std::shared_ptr<annotations::AnnotationNode> annotation);
        bool hasAnnotation(const std::string& annotationName) const;
        std::shared_ptr<annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const;

        // MYT-110: per-parameter annotation accessors. The outer vector is
        // kept sized to match parameters; empty inner vectors are allowed.
        const std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>>& getParameterAnnotations() const;
        void setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>> annotationsByIndex);
        const std::vector<std::shared_ptr<annotations::AnnotationNode>>& getParameterAnnotations(size_t paramIndex) const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
