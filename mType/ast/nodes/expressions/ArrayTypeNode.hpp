#pragma once
#include "../../ASTNode.hpp"
#include "../../../parser/TypeParser.hpp"

namespace ast::nodes::expressions
{
    /**
     * AST node representing array type declarations: int[], string[][], T[]
     * Used in variable declarations, function parameters, and return types.
     */
    class ArrayTypeNode : public ASTNode
    {
    private:
        parser::TypeInfo elementTypeInfo;    // Type of array elements
        int dimensions;                      // Number of array dimensions (1 for int[], 2 for int[][], etc.)
        bool isGenericParameter;             // True if this is T[], false if concrete type like int[]

    public:
        /**
         * Constructor for concrete array types (int[], string[][])
         * @param elemTypeInfo Type information for array elements
         * @param dims Number of array dimensions
         * @param loc Source location for error reporting
         */
        explicit ArrayTypeNode(const parser::TypeInfo& elemTypeInfo,
                             int dims = 1,
                             const SourceLocation& loc = SourceLocation());

        /**
         * Constructor for generic array types (T[], E[])
         * @param genericTypeName Name of the generic type parameter
         * @param dims Number of array dimensions
         * @param loc Source location for error reporting
         */
        explicit ArrayTypeNode(const std::string& genericTypeName,
                             int dims = 1,
                             const SourceLocation& loc = SourceLocation());

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
         * Gets the number of array dimensions
         * @return Dimension count (1 for T[], 2 for T[][], etc.)
         */
        int getDimensions() const;

        /**
         * Checks if this represents a generic type parameter array
         * @return true if generic (T[]), false if concrete (int[])
         */
        bool isGeneric() const;

        /**
         * Gets the generic type parameter name (only valid for generic arrays)
         * @return Generic parameter name (T, E, etc.)
         */
        std::string getGenericTypeName() const;

        /**
         * Creates a string representation of the array type
         * @return String like "int[]", "string[][]", "T[]"
         */
        std::string toString() const;

        /**
         * Converts this array type to equivalent Array<T> TypeInfo
         * @return TypeInfo representing Array<elementType>
         */
        parser::TypeInfo toCollectionType() const;

        /**
         * Visitor pattern implementation
         * @param visitor The visitor to accept
         * @return Result of visitor operation
         */
        Value accept(ASTVisitor<Value>& visitor) override;

    private:
        std::string genericTypeName;  // Name for generic type parameters
    };
}