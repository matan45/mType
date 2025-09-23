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
        std::vector<std::string> genericTypeArguments; // For Class::method<Type>()

    public:
        explicit MethodCallNode(std::unique_ptr<ASTNode> obj, const std::string& method,
                                std::vector<std::unique_ptr<ASTNode>> args, bool isStatic = false,
                                const std::vector<std::string>& genericArgs = {},
                                const SourceLocation& loc = SourceLocation());

        ASTNode* getObject() const;
        const std::string& getMethodName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getArguments() const;
        bool getIsStaticCall() const;
        const std::vector<std::string>& getGenericTypeArguments() const;
        bool hasGenericTypeArguments() const;

        void setObject(std::unique_ptr<ASTNode> obj);
        void setMethodName(const std::string& method);
        void setArguments(std::vector<std::unique_ptr<ASTNode>> args);
        void setIsStaticCall(bool isStatic);
        void setGenericTypeArguments(const std::vector<std::string>& genericArgs);

        void addArgument(std::unique_ptr<ASTNode> arg);
        size_t getArgumentCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
