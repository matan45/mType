#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    using namespace value;

    class DeclarationNode : public ASTNode
    {
    private:
        std::string variableName;
        ValueType type;
        std::unique_ptr<ASTNode> initializer;
        bool isFinalVar;
        bool isStaticVar;

    public:
        explicit DeclarationNode(const std::string& name, ValueType varType,
                               std::unique_ptr<ASTNode> init = nullptr,
                               bool final = false, bool staticVar = false,
                               const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), variableName(name), type(varType), 
              initializer(std::move(init)), isFinalVar(final), isStaticVar(staticVar) {}

        const std::string& getVariableName() const { return variableName; }
        ValueType getType() const { return type; }
        ASTNode* getInitializer() const { return initializer.get(); }
        bool isFinal() const { return isFinalVar; }
        bool isStatic() const { return isStaticVar; }

        void setInitializer(std::unique_ptr<ASTNode> init) {
            initializer = std::move(init);
        }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitDeclarationNode(this);
        }
    };
}