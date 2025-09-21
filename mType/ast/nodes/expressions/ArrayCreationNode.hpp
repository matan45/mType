#pragma once
#include "../../ASTNode.hpp"
#include "../../../parser/TypeParser.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * AST node representing array creation with size specification: new int[size]
     * Supports both static and dynamic size expressions.
     */
    class ArrayCreationNode : public ASTNode
    {
    private:
        parser::TypeInfo elementTypeInfo;           // Type of array elements (int, string, etc.)
        std::unique_ptr<ASTNode> sizeExpression;    // Expression for array size
        bool isDynamicSize;                         // True if size is an expression, false if literal

    public:
        /**
         * Constructor for array creation with dynamic size expression
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
         * Gets the size expression node
         * @return Pointer to the size expression AST node
         */
        const std::unique_ptr<ASTNode>& getSizeExpression() const;

        /**
         * Checks if the size is determined by an expression (vs. a literal)
         * @return true if size is dynamic, false if static
         */
        bool hasDynamicSize() const;

        /**
         * Sets a new size expression
         * @param sizeExpr New size expression node
         */
        void setSizeExpression(std::unique_ptr<ASTNode> sizeExpr);

        /**
         * Visitor pattern implementation
         * @param visitor The visitor to accept
         * @return Result of visitor operation
         */
        Value accept(ASTVisitor<Value>& visitor) override;
    };
}