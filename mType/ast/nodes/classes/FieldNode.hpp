#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericType.hpp"
#include "../../AccessModifier.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class FieldNode : public ASTNode
    {
    private:
        std::string name;
        std::shared_ptr<GenericType> type; // CHANGED: From ValueType to GenericType
        std::unique_ptr<ASTNode> initialValue;
        bool isStatic;
        bool isFinal;
        AccessModifier accessModifier;

    public:
        // NEW: Primary constructor with GenericType support
        explicit FieldNode(const std::string& fieldName,
                           std::shared_ptr<GenericType> fieldType,
                           std::unique_ptr<ASTNode> initValue = nullptr,
                           bool isStaticField = false, bool isFinalField = false,
                           AccessModifier modifier = AccessModifier::PRIVATE,
                           const SourceLocation& loc = SourceLocation());

        // Backward compatibility constructor with ValueType
        explicit FieldNode(const std::string& fieldName, ValueType fieldType,
                           std::unique_ptr<ASTNode> initValue = nullptr,
                           bool isStaticField = false, bool isFinalField = false,
                           AccessModifier modifier = AccessModifier::PRIVATE,
                           const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;

        // NEW: Generic-aware getter
        std::shared_ptr<GenericType> getGenericType() const;

        // Legacy getter for backward compatibility
        ValueType getType() const;

        ASTNode* getInitialValue() const;
        bool getIsStatic() const;
        bool getIsFinal() const;

        void setName(const std::string& fieldName);

        // NEW: Generic-aware setter
        void setGenericType(std::shared_ptr<GenericType> fieldType);

        // Legacy setter for backward compatibility
        void setType(ValueType fieldType);

        void setInitialValue(std::unique_ptr<ASTNode> initValue);
        void setIsStatic(bool isStaticField);
        void setIsFinal(bool isFinalField);

        AccessModifier getAccessModifier() const;
        void setAccessModifier(AccessModifier modifier);

        bool hasInitialValue() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
