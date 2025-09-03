#pragma once
#include "../../ASTNode.hpp"
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
        ValueType type;
        std::unique_ptr<ASTNode> initialValue;
        bool isStatic;
        bool isFinal;

    public:
        explicit FieldNode(const std::string& fieldName, ValueType fieldType,
                  std::unique_ptr<ASTNode> initValue = nullptr,
                  bool isStaticField = false, bool isFinalField = false,
                  const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        ValueType getType() const;
        ASTNode* getInitialValue() const;
        bool getIsStatic() const;
        bool getIsFinal() const;

        void setName(const std::string& fieldName);
        void setType(ValueType fieldType);
        void setInitialValue(std::unique_ptr<ASTNode> initValue);
        void setIsStatic(bool isStaticField);
        void setIsFinal(bool isFinalField);

        bool hasInitialValue() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
