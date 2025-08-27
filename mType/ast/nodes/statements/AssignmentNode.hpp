#pragma once
#include "../../ASTNode.hpp"
#include "../../vlaue/ValueType.hpp"
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
        bool isDeclaration;
        bool isFinal;
        bool isStatic;

    public:
        explicit AssignmentNode(const std::string& varName, std::unique_ptr<ASTNode> val, ValueType type = ValueType::VOID,
                       bool isDecl = false, bool isFinalVar = false, bool isStaticVar = false,
                       const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), variableName(varName), value(std::move(val)), variableType(type),
              isDeclaration(isDecl), isFinal(isFinalVar), isStatic(isStaticVar) {}

        const std::string& getVariableName() const { return variableName; }
        ASTNode* getValue() const { return value.get(); }
        ValueType getVariableType() const { return variableType; }
        bool getIsDeclaration() const { return isDeclaration; }
        bool getIsFinal() const { return isFinal; }
        bool getIsStatic() const { return isStatic; }

        void setVariableName(const std::string& varName) { variableName = varName; }
        void setValue(std::unique_ptr<ASTNode> val) { value = std::move(val); }
        void setVariableType(ValueType type) { variableType = type; }
        void setIsDeclaration(bool isDecl) { isDeclaration = isDecl; }
        void setIsFinal(bool isFinalVar) { isFinal = isFinalVar; }
        void setIsStatic(bool isStaticVar) { isStatic = isStaticVar; }

        Value accept(ASTVisitor<Value>& visitor) override {
            if (isDeclaration) {
                return visitor.visitDeclarationNode(this);
            }
            return visitor.visitAssignmentNode(this);
        }
    };
}
