#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    using namespace value;

    class AssignmentNode : public ASTNode
    {
    private:
        std::string variableName;
        std::unique_ptr<ASTNode> value;
        ValueType variableType;
        std::string className;  // Store class name for OBJECT types
        bool isFinal;
        bool isStatic;

    public:
        explicit AssignmentNode(const std::string& varName, std::unique_ptr<ASTNode> val,
                                ValueType type = ValueType::VOID,
                                const std::string& clsName = "",
                                bool isFinalVar = false, bool isStaticVar = false,
                                const SourceLocation& loc = SourceLocation());

        const std::string& getVariableName() const;
        ASTNode* getValue() const;
        ValueType getVariableType() const;
        const std::string& getClassName() const;
        bool getIsFinal() const;
        bool getIsStatic() const;

        void setVariableName(const std::string& varName);
        void setValue(std::unique_ptr<ASTNode> val);
        void setVariableType(ValueType type);
        void setClassName(const std::string& clsName);
        void setIsFinal(bool isFinalVar);
        void setIsStatic(bool isStaticVar);

        Value accept(ASTVisitor<Value>& visitor) override;
        
    };
}
