#pragma once
#include <string>
#include "../../value/ValueType.hpp"

namespace runtimeTypes::global
{
    using namespace value;

    class VariableDefinition
    {
    private:
        std::string name;
        ValueType type;
        Value value;
        bool isFinalVariable;
        std::string className; // For object types, stores the expected class name

    public:
        const std::string& getName() const { return name; }

        explicit VariableDefinition(const std::string& n, ValueType t, const Value& v = {}, bool final = false)
            : name(n), type(t), value(v), isFinalVariable(final), className("")
        {
        }

        explicit VariableDefinition(const std::string& n, ValueType t, const Value& v, bool final, const std::string& clsName)
            : name(n), type(t), value(v), isFinalVariable(final), className(clsName)
        {
        }

        const ValueType& getType() const;

        void setType(const ValueType& t);
        
        const Value& getValue() const;

        void setValue(const Value& v);

        bool isFinal() const;

        void setIsFinal(bool f);
        
        const std::string& getClassName() const;
        void setClassName(const std::string& clsName);
        
    };
}
