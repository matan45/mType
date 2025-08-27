#pragma once
#include "../ast/ASTVisitor.hpp"
#include "../vlaue/ValueType.hpp"

namespace evaluator
{
    using namespace ast;
    using namespace value;

    class Evaluator : public ASTVisitor<Value>
    {
    public:
    };
}
