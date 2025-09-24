#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * AST node representing array literal syntax: [1, 2, 3] or ["a", "b", "c"]
     * Supports both primitive and object array literals
     */
    class ArrayLiteralNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> elements;

    public:
        /**
         * Constructor for array literal with elements
         * @param elems Vector of expression nodes that represent array elements
         * @param loc Source location for error reporting
         */
        explicit ArrayLiteralNode(std::vector<std::unique_ptr<ASTNode>> elems,
                                const errors::SourceLocation& loc = errors::SourceLocation());

        /**
         * Gets the element expression nodes
         * @return Vector of pointers to element expression AST nodes
         */
        const std::vector<std::unique_ptr<ASTNode>>& getElements() const;

        /**
         * Gets the number of elements
         * @return Number of array elements
         */
        size_t getElementCount() const;

        /**
         * Visitor pattern implementation
         * @param visitor The visitor to accept
         * @return Result of visitor operation
         */
        Value accept(ASTVisitor<Value>& visitor) override;
    };
}