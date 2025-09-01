#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::functions
{
    using namespace value;

    class FunctionNode : public ASTNode
    {
    private:
        std::string name;
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::unique_ptr<ASTNode> body;

    public:
        explicit FunctionNode(const std::string& funcName, ValueType retType,
                     std::vector<std::pair<std::string, ValueType>> params,
                     std::unique_ptr<ASTNode> funcBody,
                     const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), name(funcName), returnType(retType), parameters(std::move(params)),
              body(std::move(funcBody))
        {
        }

        const std::string& getName() const { return name; }
        ValueType getReturnType() const { return returnType; }
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        std::unique_ptr<ASTNode> getBody() const { return body.get(); }

        void setName(const std::string& funcName) { name = funcName; }
        void setReturnType(ValueType retType) { returnType = retType; }
        void setParameters(std::vector<std::pair<std::string, ValueType>> params) { parameters = std::move(params); }
        void setBody(std::unique_ptr<ASTNode> funcBody) { body = std::move(funcBody); }

        size_t getParameterCount() const { return parameters.size(); }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitFunctionNode(this);
        }
    };
}
