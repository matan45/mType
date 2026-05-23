#pragma once
#include <string>
#include "../../value/ValueType.hpp"
#include "../../types/UnifiedType.hpp"

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

        // MYT-359: declared type for LSP receiver-type inference (mirrors MYT-357
        // FieldDefinition). Populated by SymbolRegistrationVisitor from the
        // local-decl / for-each / parameter declared type.
        ::types::UnifiedTypePtr unifiedType;

    public:
        const std::string& getName() const { return name; }

        explicit VariableDefinition(const std::string& n, ValueType t, const Value& v = {}, bool final = false)
            : name(n), type(t), value(v), isFinalVariable(final), className(""), unifiedType(nullptr)
        {
        }

        explicit VariableDefinition(const std::string& n, ValueType t, const Value& v, bool final, const std::string& clsName)
            : name(n), type(t), value(v), isFinalVariable(final), className(clsName), unifiedType(nullptr)
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

        // MYT-359: unified type accessors mirror FieldDefinition's API.
        ::types::UnifiedTypePtr getUnifiedType() const { return unifiedType; }
        void setUnifiedType(::types::UnifiedTypePtr type) { unifiedType = std::move(type); }
        bool hasUnifiedType() const { return unifiedType != nullptr; }
    };
}
