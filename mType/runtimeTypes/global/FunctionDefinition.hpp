#pragma once
#include "../../vlaue/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace ast;
    
    class FunctionDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        ASTNode* body;
    public:
    };
}
