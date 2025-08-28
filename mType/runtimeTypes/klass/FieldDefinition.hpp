#pragma once
#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;

    class FieldDefinition : public Definition
    {
    private:
        ValueType type;
        Value value;
        bool isStatic;
        bool isFinal;

    public:
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false)
            : Definition(n), type(t), value(v),
              isStatic(stat), isFinal(fin)
        {
        }
    };
}
