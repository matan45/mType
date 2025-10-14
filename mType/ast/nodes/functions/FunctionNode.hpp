#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include "../../GenericType.hpp"
#include "../../VisibilityModifier.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../value/ParameterType.hpp"
#include <string>
#include <vector>
#include <memory>

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

        const std::string& getName() const;

        // NEW: Generic-aware getters
        std::shared_ptr<GenericType> getGenericReturnType() const;
        const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& getGenericParameters() const;

        // Legacy getters for backward compatibility
        ValueType getReturnType() const;
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // NEW: Get parameters as ParameterType (preserves class/interface information)
        std::vector<std::pair<std::string, ParameterType>> getParameterTypes() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const;

        // NEW: Generic-related methods
        const std::vector<GenericTypeParameter>& getGenericTypeParameters() const;
        void setGenericTypeParameters(const std::vector<GenericTypeParameter>& generics);
        void addGenericTypeParameter(const GenericTypeParameter& param);
        size_t getGenericTypeParameterCount() const;
        bool isGeneric() const { return !genericParameters.empty(); }

        // NEW: Async-related methods
        bool getIsAsync() const { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Visibility modifier methods (for import/export system)
        VisibilityModifier getVisibility() const;
        void setVisibility(VisibilityModifier vis);
        bool isPublic() const;
        bool isPrivate() const;

        void setName(const std::string& funcName);
        void setGenericReturnType(std::shared_ptr<GenericType> retType);
        void setGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);

        // Legacy setters for backward compatibility
        void setReturnType(ValueType retType);
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params);

        void setBody(std::shared_ptr<ASTNode> funcBody);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
