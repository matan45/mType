#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class MethodNode : public ASTNode
    {
    private:
        std::string name;
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::unique_ptr<ASTNode> body;
        bool isStatic;

    public:
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   std::vector<std::pair<std::string, ValueType>> params,
                   std::unique_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), name(methodName), returnType(retType), parameters(std::move(params)),
              body(std::move(methodBody)), isStatic(isStaticMethod)
        {
        }

        const std::string& getName() const { return name; }
        ValueType getReturnType() const { return returnType; }
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        ASTNode* getBody() const { return body.get(); }
        bool getIsStatic() const { return isStatic; }

        void setName(const std::string& methodName) { name = methodName; }
        void setReturnType(ValueType retType) { returnType = retType; }
        void setParameters(std::vector<std::pair<std::string, ValueType>> params) { parameters = std::move(params); }
        void setBody(std::unique_ptr<ASTNode> methodBody) { body = std::move(methodBody); }
        void setIsStatic(bool isStaticMethod) { isStatic = isStaticMethod; }

        size_t getParameterCount() const { return parameters.size(); }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitMethodNode(this);
        }
    };
}
