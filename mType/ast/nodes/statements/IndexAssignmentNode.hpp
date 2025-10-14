#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class IndexAssignmentNode : public ASTNode
    {
    private:
        std::shared_ptr<ASTNode> object;
        std::shared_ptr<ASTNode> index;
        std::shared_ptr<ASTNode> value;

    public:
        // Constructor accepting shared_ptr
        explicit IndexAssignmentNode(std::shared_ptr<ASTNode> obj, std::shared_ptr<ASTNode> idx, std::shared_ptr<ASTNode> val,
                             const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit IndexAssignmentNode(std::unique_ptr<ASTNode> obj, std::unique_ptr<ASTNode> idx, std::unique_ptr<ASTNode> val,
                             const SourceLocation& loc = SourceLocation());

        // For code that just needs to read
        [[nodiscard]] ASTNode* getObject() const noexcept;
        [[nodiscard]] ASTNode* getIndex() const noexcept;
        [[nodiscard]] ASTNode* getValue() const noexcept;

        // Safe getters - return shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getObjectShared() const;
        [[nodiscard]] std::shared_ptr<ASTNode> getIndexShared() const;
        [[nodiscard]] std::shared_ptr<ASTNode> getValueShared() const;

        void setObject(std::shared_ptr<ASTNode> obj);
        void setIndex(std::shared_ptr<ASTNode> idx);
        void setValue(std::shared_ptr<ASTNode> val);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}