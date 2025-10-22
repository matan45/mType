#pragma once
#include "../../ASTNode.hpp"
#include "../../VisibilityModifier.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    class AssignmentNode : public ASTNode
    {
    private:
        std::string variableName;
        std::unique_ptr<ASTNode> value;
        value::ValueType variableType;
        std::string className;  // Store class name for OBJECT types
        bool isFinal;
        bool isStatic;
        VisibilityModifier visibility;

    public:
        explicit AssignmentNode(const std::string& varName, std::unique_ptr<ASTNode> val,
                                value::ValueType type = value::ValueType::VOID,
                                const std::string& clsName = "",
                                bool isFinalVar = false, bool isStaticVar = false,
                                const SourceLocation& loc = SourceLocation());

        const std::string& getVariableName() const;
        ASTNode* getValue() const;
        value::ValueType getVariableType() const;
        const std::string& getClassName() const;
        bool getIsFinal() const;
        bool getIsStatic() const;
        VisibilityModifier getVisibility() const;

        void setVariableName(const std::string& varName);
        void setValue(std::unique_ptr<ASTNode> val);
        void setVariableType(value::ValueType type);
        void setClassName(const std::string& clsName);
        void setIsFinal(bool isFinalVar);
        void setIsStatic(bool isStaticVar);
        void setVisibility(VisibilityModifier vis);

        value::Value accept(ASTVisitor<value::Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
