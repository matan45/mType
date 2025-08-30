#include "VariableDefinition.hpp"

namespace runtimeTypes::global
{
    const ValueType& VariableDefinition::getType() const
    {
        return type;
    }

    void VariableDefinition::setType(const ValueType& t)
    {
        type = t;
    }

    const Value& VariableDefinition::getValue() const
    {
        return value;
    }

    void VariableDefinition::setValue(const Value& v)
    {
        value = v;
    }

    bool VariableDefinition::getIsFinal() const
    {
        return isFinalVariable;
    }

    bool VariableDefinition::isFinal() const
    {
        return isFinalVariable;
    }

    void VariableDefinition::setIsFinal(bool f)
    {
        isFinalVariable = f;
    }

    VariableDefinition* VariableDefinition::inNamespace(const std::vector<std::string>& context)
    {
        setNamespaceContext(context);
        return this;
    }

    VariableDefinition* VariableDefinition::inNamespace(const std::string& ns)
    {
        addNamespaceLevel(ns);
        return this;
    }
}
