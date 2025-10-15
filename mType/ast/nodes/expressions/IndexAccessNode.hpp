#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    class IndexAccessNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> collection;
        std::unique_ptr<ASTNode> index;

    public:
        explicit IndexAccessNode(std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> index, const SourceLocation& loc = SourceLocation());

        ASTNode* getCollection() const;
        ASTNode* getIndex() const;
        void setCollection(std::unique_ptr<ASTNode> collection);
        void setIndex(std::unique_ptr<ASTNode> index);

        // Transfer ownership methods for parser use
        [[nodiscard]] std::unique_ptr<ASTNode> transferCollectionOwnership();
        [[nodiscard]] std::unique_ptr<ASTNode> transferIndexOwnership();

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}