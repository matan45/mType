#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class ConstructorNode : public ASTNode
    {
    private:
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::shared_ptr<ASTNode> body;

    public:
        // Constructor accepting shared_ptr
        explicit ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                 std::shared_ptr<ASTNode> constructorBody,
                                 const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                 std::unique_ptr<ASTNode> constructorBody,
                                 const SourceLocation& loc = SourceLocation());

        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;

        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;

        void setParameters(std::vector<std::pair<std::string, ValueType>> params);
        void setBody(std::shared_ptr<ASTNode> constructorBody);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
