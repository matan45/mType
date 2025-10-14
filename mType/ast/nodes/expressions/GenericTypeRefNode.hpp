#pragma once

#include "../../ASTNode.hpp"
#include "../../GenericType.hpp"
#include "../../../value/ValueType.hpp"

namespace ast::nodes::expressions
{
    using namespace value;
    /**
     * AST node representing a reference to a generic type.
     * Used in contexts like field declarations (T value), parameter types (T param),
     * and return types (function get(): T).
     */
    class GenericTypeRefNode : public ASTNode
    {
    private:
        std::shared_ptr<GenericType> type;

    public:
        /**
         * Constructor for generic type reference node.
         * @param genericType The generic type being referenced
         * @param loc Source location where this type reference appears
         */
        explicit GenericTypeRefNode(std::shared_ptr<GenericType> genericType,
                                    const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), type(genericType)
        {
        }

        /**
         * Gets the generic type being referenced.
         * @return Shared pointer to the GenericType
         */
        std::shared_ptr<GenericType> getType() const { return type; }

        /**
         * Sets the generic type being referenced.
         * @param genericType The new generic type
         */
        void setType(std::shared_ptr<GenericType> genericType) { type = genericType; }

        /**
         * Gets a string representation of this type reference.
         * @return String representation like "T", "Array<T>", "Map<K,V>"
         */
        std::string getTypeString() const
        {
            return type ? type->toString() : "unknown";
        }

        /**
         * Checks if this references a generic type parameter.
         * @return true if this is a type parameter (T, E, etc.), false if concrete
         */
        bool isGenericParameter() const
        {
            return type && type->isGenericParameter();
        }

        /**
         * Checks if this references a parameterized type.
         * @return true if this has type arguments (Array<T>), false otherwise
         */
        bool isParameterized() const
        {
            return type && type->isParameterized();
        }

        /**
         * Accept method for visitor pattern.
         * @param visitor The AST visitor
         * @return Result of visiting this node
         */
        value::Value accept(ASTVisitor<value::Value>& visitor) override;

        /**
         * Gets the node type for debugging and logging.
         * @return String identifier for this node type
         */
        std::string getNodeType() const
        {
            return "GenericTypeRefNode";
        }

        /**
         * Creates a string representation of this node for debugging.
         * @return Debug string
         */
        std::string toString() const
        {
            return "GenericTypeRef(" + getTypeString() + ")";
        }

        /**
         * Creates a copy of this node.
         * @return Unique pointer to a new copy of this node
         */
        std::unique_ptr<ASTNode> clone() const override
        {
            auto clonedType = type ? std::make_shared<GenericType>(*type) : nullptr;
            return std::make_unique<GenericTypeRefNode>(clonedType, getLocation());
        }
    };
}
