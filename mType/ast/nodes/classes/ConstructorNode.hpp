#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../value/ParameterType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class SuperConstructorCallNode; // Forward declaration

    class ConstructorNode : public ASTNode
    {
    private:
        std::vector<std::pair<std::string, ParameterType>> parametersWithTypes;
        std::shared_ptr<ASTNode> body;
        std::unique_ptr<SuperConstructorCallNode> superInitializer;

        // Cached computed property - lazily computed from parametersWithTypes
        mutable std::vector<std::pair<std::string, ValueType>> cachedParameters;
        mutable bool parametersCacheValid = false;

    public:
        // Constructor with ParameterType (preserves class/interface information)
        explicit ConstructorNode(std::vector<std::pair<std::string, ParameterType>> params,
                                 std::shared_ptr<ASTNode> constructorBody,
                                 const SourceLocation& loc = SourceLocation());

        // Computed property - derives from parametersWithTypes
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // NEW: Get parameters with full type information
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const;
        bool hasParametersWithTypes() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;

        void setBody(std::shared_ptr<ASTNode> constructorBody);

        // Super initializer accessors
        void setSuperInitializer(std::unique_ptr<SuperConstructorCallNode> superCall);
        [[nodiscard]] SuperConstructorCallNode* getSuperInitializer() const noexcept;
        [[nodiscard]] bool hasSuperInitializer() const noexcept;

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
