#pragma once
#include "../../value/ValueType.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;

    class FieldDefinition : public Definition
    {
    private:
        ValueType type;
        Value value;
        bool isStaticField;
        bool isFinalField;
        ast::AccessModifier accessModifier;

    public:
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), accessModifier(modifier)
        {
        }

        const ValueType& getType() const { return type; }
        void setType(const ValueType& t) { type = t; }
        
        const Value& getValue() const { return value; }
        void setValue(const Value& v) { value = v; }
        
        bool isStatic() const { return isStaticField; }
        void setStatic(bool stat) { isStaticField = stat; }
        
        bool isFinal() const { return isFinalField; }
        void setFinal(bool fin) { isFinalField = fin; }

        ast::AccessModifier getAccessModifier() const { return accessModifier; }
        void setAccessModifier(ast::AccessModifier modifier) { accessModifier = modifier; }
    };
}
