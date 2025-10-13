#pragma once
#include "../../value/ValueType.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../ast/GenericType.hpp"
#include "../Definition.hpp"
#include <memory>

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

        // NEW: Generic type information for precise type handling
        std::shared_ptr<ast::GenericType> genericType;

    public:
        // Legacy constructor for backward compatibility
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), isInitializedField(false), accessModifier(modifier),
              genericType(nullptr)
        {
        }

        // NEW: Constructor with generic type information
        explicit FieldDefinition(const std::string& n, ValueType t,
                        std::shared_ptr<ast::GenericType> genType,
                        const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), isInitializedField(false), accessModifier(modifier),
              genericType(genType)
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

        // NEW: Generic type information getters and setters
        std::shared_ptr<ast::GenericType> getGenericType() const { return genericType; }
        void setGenericType(std::shared_ptr<ast::GenericType> genType) { genericType = genType; }
        bool hasGenericType() const { return genericType != nullptr; }
    };
}
