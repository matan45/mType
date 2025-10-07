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
        bool isInitializedField; // Track if final field has been initialized
        ast::AccessModifier accessModifier;

    public:
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), isInitializedField(false), accessModifier(modifier)
        {
        }

        const ValueType& getType() const { return type; }
        void setType(const ValueType& t) { type = t; }

        const Value& getValue() const { return value; }
        void setValue(const Value& v) {
            value = v;
            if (isFinalField) {
                isInitializedField = true; // Mark as initialized when setting a final field
            }
        }

        bool isStatic() const { return isStaticField; }
        void setStatic(bool stat) { isStaticField = stat; }

        bool isFinal() const { return isFinalField; }
        void setFinal(bool fin) { isFinalField = fin; }

        bool isInitialized() const { return isInitializedField; }

        ast::AccessModifier getAccessModifier() const { return accessModifier; }
        void setAccessModifier(ast::AccessModifier modifier) { accessModifier = modifier; }
    };
}
