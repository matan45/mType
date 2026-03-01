#pragma once
#include "../../value/ValueType.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../types/UnifiedType.hpp"
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

        // Type information for precise type handling (UnifiedType)
        ::types::UnifiedTypePtr unifiedType;

    public:
        // Legacy constructor for backward compatibility
        explicit FieldDefinition(const std::string& n, ValueType t, const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), isInitializedField(false), accessModifier(modifier),
              unifiedType(nullptr)
        {
        }

        // Constructor with unified type information
        explicit FieldDefinition(const std::string& n, ValueType t,
                        ::types::UnifiedTypePtr uType,
                        const Value& v = {},
                        bool stat = false, bool fin = false,
                        ast::AccessModifier modifier = ast::AccessModifier::PRIVATE)
            : Definition(n), type(t), value(v),
              isStaticField(stat), isFinalField(fin), isInitializedField(false), accessModifier(modifier),
              unifiedType(std::move(uType))
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

        // Unified type information getters and setters
        ::types::UnifiedTypePtr getUnifiedType() const { return unifiedType; }
        void setUnifiedType(::types::UnifiedTypePtr type) { unifiedType = std::move(type); }
        bool hasUnifiedType() const { return unifiedType != nullptr; }

        // Reflection support: get modifier flags as bitmask
        // PUBLIC=1, PRIVATE=2, PROTECTED=4, STATIC=8, FINAL=16
        int getModifierFlags() const {
            int flags = 0;
            switch (accessModifier) {
                case ast::AccessModifier::PUBLIC: flags |= 1; break;
                case ast::AccessModifier::PRIVATE: flags |= 2; break;
                case ast::AccessModifier::PROTECTED: flags |= 4; break;
            }
            if (isStaticField) flags |= 8;
            if (isFinalField) flags |= 16;
            return flags;
        }
    };
}
