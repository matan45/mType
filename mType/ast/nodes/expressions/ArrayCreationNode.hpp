#pragma once
#include "../../ASTNode.hpp"
#include "../../../parser/TypeParser.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * AST node representing array creation with size specification: new int[size] or new int[size1][size2]
     * Supports both static and dynamic size expressions for multidimensional arrays.
     */
    class ArrayCreationNode : public ASTNode
    {
    private:
        parser::TypeInfo elementTypeInfo;                    // Type of array elements (int, string, etc.)
        std::vector<std::unique_ptr<ASTNode>> sizeExpressions; // Expressions for array sizes (one per dimension)
        std::vector<bool> isDynamicSizes;                    // True if size is an expression, false if literal

    public:
        /**
         * Constructor for array creation with dynamic size expressions
         * @param elemTypeInfo Type information for array elements
         * @param sizeExprs Vector of expression nodes that evaluate to array sizes (one per dimension)
         * @param loc Source location for error reporting
         */
        explicit ArrayCreationNode(const parser::TypeInfo& elemTypeInfo,
                                 std::vector<std::unique_ptr<ASTNode>> sizeExprs,
                                 const errors::SourceLocation& loc = errors::SourceLocation());

        /**
         * Constructor for single-dimension array (backward compatibility)
         * @param elemTypeInfo Type information for array elements
         * @param sizeExpr Expression node that evaluates to array size
         * @param loc Source location for error reporting
         */
        explicit ArrayCreationNode(const parser::TypeInfo& elemTypeInfo,
                                 std::unique_ptr<ASTNode> sizeExpr,
                                 const errors::SourceLocation& loc = errors::SourceLocation());

        /**
         * Gets the element type information
         * @return TypeInfo for array elements
         */
        const parser::TypeInfo& getElementTypeInfo() const;

        /**
         * Gets the legacy element type (for backward compatibility)
         * @return ValueType of array elements
         */
        ValueType getElementType() const;

        /**
         * Gets the size expression nodes for all dimensions
         * @return Vector of pointers to size expression AST nodes
         */
        const std::vector<std::unique_ptr<ASTNode>>& getSizeExpressions() const;

        /**
         * Gets the size expression node for a specific dimension (backward compatibility)
         * @return Pointer to the first dimension size expression AST node
         */
        const std::unique_ptr<ASTNode>& getSizeExpression() const;

        /**
         * Gets the number of dimensions
         * @return Number of array dimensions
         */
        size_t getDimensionCount() const;

        /**
         * Checks if any size is determined by an expression (vs. a literal)
         * @return true if any size is dynamic, false if all static
         */
        bool hasDynamicSize() const;

        /**
         * Checks if a specific dimension has dynamic size
         * @param dimension Dimension index (0-based)
         * @return true if the dimension size is dynamic, false if static
         */
        bool hasDynamicSize(size_t dimension) const;

        /**
         * Sets a new size expression for single dimension (backward compatibility)
         * @param sizeExpr New size expression node
         */
        void setSizeExpression(std::unique_ptr<ASTNode> sizeExpr);

        /**
         * Adds a new dimension with size expression
         * @param sizeExpr Size expression node for the new dimension
         */
        void addDimension(std::unique_ptr<ASTNode> sizeExpr);

        /**
         * Visitor pattern implementation
         * @param visitor The visitor to accept
         * @return Result of visitor operation
         */
        Value accept(ASTVisitor<Value>& visitor) override;
    };
}