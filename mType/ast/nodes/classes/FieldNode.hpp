#pragma once
#include "../../ASTNode.hpp"
#include "../../../vlaue/ValueType.hpp"
#include <string>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class FieldNode : public ASTNode
    {
    private:
        std::string name;
        ValueType type;
        std::unique_ptr<ASTNode> initialValue;
        bool isStatic;
        bool isFinal;

    public:
        explicit FieldNode(const std::string& fieldName, ValueType fieldType,
                  std::unique_ptr<ASTNode> initValue = nullptr,
                  bool isStaticField = false, bool isFinalField = false,
                  const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), name(fieldName), type(fieldType), initialValue(std::move(initValue)),
              isStatic(isStaticField), isFinal(isFinalField)
        {
        }

        const std::string& getName() const { return name; }
        ValueType getType() const { return type; }
        ASTNode* getInitialValue() const { return initialValue.get(); }
        bool getIsStatic() const { return isStatic; }
        bool getIsFinal() const { return isFinal; }

        void setName(const std::string& fieldName) { name = fieldName; }
        void setType(ValueType fieldType) { type = fieldType; }
        void setInitialValue(std::unique_ptr<ASTNode> initValue) { initialValue = std::move(initValue); }
        void setIsStatic(bool isStaticField) { isStatic = isStaticField; }
        void setIsFinal(bool isFinalField) { isFinal = isFinalField; }

        bool hasInitialValue() const { return initialValue != nullptr; }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitFieldNode(this);
        }
    };
}
