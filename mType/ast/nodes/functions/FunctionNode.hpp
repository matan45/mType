#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include "../../GenericType.hpp"
#include "../../VisibilityModifier.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../value/ParameterType.hpp"
#include "../annotations/AnnotationNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace ast::nodes::functions
{
    using namespace value;

    class FunctionNode : public ASTNode
    {
    private:
        std::string name;
        std::vector<GenericTypeParameter> genericParameters;  // NEW: For generic functions
        std::shared_ptr<GenericType> returnType;              // CHANGED: From ValueType to GenericType
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> parameters;  // CHANGED: GenericType instead of ValueType
        std::shared_ptr<ASTNode> body;
        bool isAsync;  // NEW: Flag to indicate async function
        VisibilityModifier visibility;  // NEW: Top-level visibility for imports
        std::vector<std::shared_ptr<annotations::AnnotationNode>> annotations;  // NEW: Annotations for this function

        // Performance cache for legacy getParameters() - O(1) after first call
        mutable std::optional<std::vector<std::pair<std::string, ValueType>>> cachedLegacyParams;
        mutable bool paramCacheValid = false;

    public:
        // NEW: Primary constructor with generic support
        explicit FunctionNode(const std::string& funcName,
                             std::shared_ptr<GenericType> retType,
                             const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params,
                             std::shared_ptr<ASTNode> funcBody,
                             const std::vector<GenericTypeParameter>& generics = {},
                             bool async = false,
                             const SourceLocation& loc = SourceLocation());

        // Backward compatibility constructor with ValueType (shared_ptr)
        explicit FunctionNode(const std::string& funcName, ValueType retType,
                     const std::vector<std::pair<std::string, ValueType>>& params,
                     std::shared_ptr<ASTNode> funcBody,
                     bool async = false,
                     const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit FunctionNode(const std::string& funcName, ValueType retType,
                     const std::vector<std::pair<std::string, ValueType>>& params,
                     std::unique_ptr<ASTNode> funcBody,
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

        // NEW: Get parameters as ParameterType (preserves class/interface information)
        [[nodiscard]] std::vector<std::pair<std::string, ParameterType>> getParameterTypes() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const noexcept;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;

        // NEW: Generic-related methods
        [[nodiscard]] const std::vector<GenericTypeParameter>& getGenericTypeParameters() const noexcept;
        void setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics);
        void addGenericTypeParameter(const GenericTypeParameter& param);
        [[nodiscard]] size_t getGenericTypeParameterCount() const noexcept;
        [[nodiscard]] bool isGeneric() const noexcept { return !genericParameters.empty(); }

        // NEW: Async-related methods
        [[nodiscard]] bool getIsAsync() const noexcept { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Visibility modifier methods (for import/export system)
        [[nodiscard]] VisibilityModifier getVisibility() const noexcept;
        void setVisibility(VisibilityModifier vis);
        [[nodiscard]] bool isPublic() const noexcept;
        [[nodiscard]] bool isPrivate() const noexcept;

        void setName(const std::string& funcName);
        void setGenericReturnType(std::shared_ptr<GenericType> retType);
        void setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);

        // Legacy setters for backward compatibility
        void setReturnType(ValueType retType);
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params);

        void setBody(std::shared_ptr<ASTNode> funcBody);

        [[nodiscard]] size_t getParameterCount() const noexcept;

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<annotations::AnnotationNode>>& getAnnotations() const;
        void addAnnotation(std::shared_ptr<annotations::AnnotationNode> annotation);
        bool hasAnnotation(const std::string& annotationName) const;
        std::shared_ptr<annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;

    private:
        // Helper method for classifying parameter types
        ParameterType classifyParameterType(const std::shared_ptr<GenericType>& paramType) const;
    };
}
