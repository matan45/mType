#pragma once
#include "../value/ValueType.hpp"
#include "ASTVisitor.hpp"
#include "../errors/SourceLocation.hpp"

namespace ast
{
    using namespace errors;
    using namespace value;

    class ASTNode
    {
    protected:
        SourceLocation location;

    public:
        explicit ASTNode() = default;

        explicit ASTNode(const SourceLocation& loc) : location(loc)
        {
        }

        virtual ~ASTNode() = default;

        // Accept method for visitor pattern
        virtual Value accept(ASTVisitor<Value>& visitor) = 0;

        // Template version for other return types
        template <typename T>
        T accept(ASTVisitor<T>& visitor);

        const SourceLocation& getLocation() const { return location; }
    };
}
