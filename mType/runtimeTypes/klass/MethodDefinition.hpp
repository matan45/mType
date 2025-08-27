#pragma once
#include <vector>
#include <utility>
#include "../../vlaue/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"


namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class MethodDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::vector<std::pair<std::string, Value>> arguments;
        ASTNode* body;
        bool isStatic;
        bool isFinal;

    public:
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         ASTNode* b, bool s, bool f)
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStatic(s),
              isFinal(f)
        {
        }

        bool matchesArgCount(size_t argCount) const;
    };
}
