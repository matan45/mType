#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class MethodCallNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> object;
        std::string methodName;
        std::vector<std::unique_ptr<ASTNode>> arguments;
        bool isStaticCall; // For Class::method() vs obj.method()

    public:
        explicit MethodCallNode(std::unique_ptr<ASTNode> obj, const std::string& method,
                       std::vector<std::unique_ptr<ASTNode>> args, bool isStatic = false,
                       const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), object(std::move(obj)), methodName(method),
              arguments(std::move(args)), isStaticCall(isStatic)
        {
        }

        std::unique_ptr<ASTNode> getObject() const { return object; }
        const std::string& getMethodName() const { return methodName; }
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const { return arguments; }
        bool getIsStaticCall() const { return isStaticCall; }

        void setObject(std::unique_ptr<ASTNode> obj) { object = std::move(obj); }
        void setMethodName(const std::string& method) { methodName = method; }
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args) { arguments = std::move(args); }
        void setIsStaticCall(bool isStatic) { isStaticCall = isStatic; }

        void addArgument(std::unique_ptr<ASTNode> arg) { arguments.push_back(std::move(arg)); }
        size_t getArgumentCount() const { return arguments.size(); }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitMethodCallNode(this);
        }
    };
}
