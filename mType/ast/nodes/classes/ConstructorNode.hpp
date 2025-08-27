#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class ConstructorNode : public ASTNode
    {
    private:
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::unique_ptr<ASTNode> body;

    public:
        explicit ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                        std::unique_ptr<ASTNode> constructorBody,
                        const SourceLocation& loc = SourceLocation());

        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;
        ASTNode* getBody() const;

        void setParameters(std::vector<std::pair<std::string, ValueType>> params);
        void setBody(std::unique_ptr<ASTNode> constructorBody);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        
    };
}
