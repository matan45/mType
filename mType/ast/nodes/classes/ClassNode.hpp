#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class ClassNode : public ASTNode
    {
    private:
        std::string className;
        std::vector<std::unique_ptr<ASTNode>> fields;
        std::vector<std::unique_ptr<ASTNode>> constructors;
        std::vector<std::unique_ptr<ASTNode>> methods;

    public:
        explicit ClassNode(const std::string& name, const SourceLocation& loc = SourceLocation());

        const std::string& getClassName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getFields() const;
        const std::vector<std::unique_ptr<ASTNode>>& getConstructors() const;
        const std::vector<std::unique_ptr<ASTNode>>& getMethods() const;

        void setClassName(const std::string& name);

        void addField(std::unique_ptr<ASTNode> field);
        void addConstructor(std::unique_ptr<ASTNode> constructor);
        void addMethod(std::unique_ptr<ASTNode> method);

        size_t getFieldCount() const;
        size_t getConstructorCount() const;
        size_t getMethodCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        
    };
}
