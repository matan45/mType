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
                                const SourceLocation& loc = SourceLocation());

        ASTNode* getObject() const;
        const std::string& getMethodName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        bool getIsStaticCall() const;

        void setObject(std::unique_ptr<ASTNode> obj);
        void setMethodName(const std::string& method);
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args);
        void setIsStaticCall(bool isStatic);

        void addArgument(std::unique_ptr<ASTNode> arg);
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
