#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include "../../GenericType.hpp"
#include "../../AccessModifier.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

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
        AccessModifier accessModifier;

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

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
