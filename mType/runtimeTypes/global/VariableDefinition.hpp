#pragma once
#include "../../value/ValueType.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::global
{
    using namespace value;

    class VariableDefinition : public Definition
    {
    private:
        ValueType type;
        Value value;
        bool isFinal;

    public:
        explicit VariableDefinition(const std::string& n, ValueType t, const Value& v = {}, bool final = false)
            : Definition(n), type(t), value(v), isFinal(final)
        {
        }

        const ValueType& getType() const;

        void setType(const ValueType& t);
        
        const Value& getValue() const;

        void setValue(const Value& v);
        
        bool getIsFinal() const;
        
        void setIsFinal(bool f);

        VariableDefinition* inNamespace(const std::vector<std::string>& context);
        VariableDefinition* inNamespace(const std::string& ns);
        
    };
}
