#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class ClassNode : public ASTNode
    {
    private:
        std::string className;
        std::vector<GenericTypeParameter> genericParameters; // NEW: Generic type parameters
        std::string parentClassName; // NEW: Parent class for inheritance
        std::vector<std::unique_ptr<ASTNode>> fields;
        std::vector<std::unique_ptr<ASTNode>> constructors;
        std::vector<std::unique_ptr<ASTNode>> methods;
        std::vector<std::string> implementedInterfaces; // NEW
    public:
        // Backward compatibility constructor (most common usage)
        explicit ClassNode(const std::string& name, const SourceLocation& loc = SourceLocation());

        // NEW: Primary constructor with generic parameters
        explicit ClassNode(const std::string& name,
                           const std::vector<GenericTypeParameter>& generics,
                           const std::vector<std::string>& interfaces = {},
                           const SourceLocation& loc = SourceLocation());

        // NEW: Constructor with inheritance support
        explicit ClassNode(const std::string& name,
                           const std::vector<GenericTypeParameter>& generics,
                           const std::string& parentClass,
                           const std::vector<std::string>& interfaces = {},
                           const SourceLocation& loc = SourceLocation());

        const std::string& getClassName() const;
        const std::vector<std::unique_ptr<ASTNode>>& getFields() const;
        const std::vector<std::unique_ptr<ASTNode>>& getConstructors() const;
        const std::vector<std::unique_ptr<ASTNode>>& getMethods() const;

        // NEW: Generic-related methods
        const std::vector<GenericTypeParameter>& getGenericParameters() const;
        void setGenericParameters(const std::vector<GenericTypeParameter>& generics);
        void addGenericParameter(const GenericTypeParameter& param);
        size_t getGenericParameterCount() const;
        bool isGeneric() const;

        // Helper to get full generic class name (e.g., "Box<T>")
        std::string getFullClassName() const;

        void setClassName(const std::string& name);

        void addField(std::unique_ptr<ASTNode> field);
        void addConstructor(std::unique_ptr<ASTNode> constructor);
        void addMethod(std::unique_ptr<ASTNode> method);

        size_t getFieldCount() const;
        size_t getConstructorCount() const;
        size_t getMethodCount() const;

        const std::vector<std::string>& getImplementedInterfaces() const;

        // NEW: Inheritance methods
        const std::string& getParentClassName() const;
        void setParentClassName(const std::string& parent);
        bool hasParentClass() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
