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
                           const SourceLocation& loc = SourceLocation());

        // Backward compatibility constructor with ValueType
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   const std::vector<std::pair<std::string, ValueType>>& params,
                   std::shared_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   AccessModifier modifier = AccessModifier::PRIVATE,
                   const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   const std::vector<std::pair<std::string, ValueType>>& params,
                   std::unique_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   AccessModifier modifier = AccessModifier::PRIVATE,
                   const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;

        // NEW: Generic-aware getters
        std::shared_ptr<GenericType> getGenericReturnType() const;
        const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& getGenericParameters() const;

        // Legacy getters for backward compatibility
        ValueType getReturnType() const;
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const;

        bool getIsStatic() const;

        // NEW: Generic-related methods
        const std::vector<GenericTypeParameter>& getGenericTypeParameters() const;
        void setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics);
        void addGenericTypeParameter(const GenericTypeParameter& param);
        size_t getGenericTypeParameterCount() const;
        bool isGeneric() const { return !genericParameters.empty(); }

        void setName(const std::string& methodName);
        void setGenericReturnType(std::shared_ptr<GenericType> retType);
        void setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);

        // Legacy setters for backward compatibility
        void setReturnType(ValueType retType);
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params);

        void setBody(std::shared_ptr<ASTNode> methodBody);
        void setIsStatic(bool isStaticMethod);

        AccessModifier getAccessModifier() const;
        void setAccessModifier(AccessModifier modifier);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
