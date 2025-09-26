#pragma once
#include "../../value/ValueType.hpp"
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

    public:
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin)
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
    };
}
