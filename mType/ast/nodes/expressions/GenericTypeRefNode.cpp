#include "GenericTypeRefNode.hpp"
#include "../../ASTVisitor.hpp"

namespace ast::nodes::expressions
{
    value::Value GenericTypeRefNode::accept(ASTVisitor<value::Value>& visitor)
    {
        // For now, generic type references don't have a direct value representation
        // during evaluation. They are primarily used during type checking and
        // semantic analysis phases.

        // Return a null value as type references are not evaluated to runtime values
        return value::Value(nullptr);
    }
}