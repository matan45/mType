#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <string>

namespace ast::nodes::namespaces
{
    class QualifiedNameNode : public ASTNode
    {
    private:
        std::vector<std::string> qualifiers; // e.g., ["math", "geometry", "area"]

    public:
        explicit QualifiedNameNode(const std::vector<std::string>& quals, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), qualifiers(quals) {}

        explicit QualifiedNameNode(const std::string& simpleName, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), qualifiers({simpleName}) {}

        const std::vector<std::string>& getQualifiers() const { return qualifiers; }
        std::string getSimpleName() const { return qualifiers.empty() ? "" : qualifiers.back(); }
        std::string getFullName() const;

        void setQualifiers(const std::vector<std::string>& quals) { qualifiers = quals; }
        void addQualifier(const std::string& qualifier) { qualifiers.push_back(qualifier); }

        bool isQualified() const { return qualifiers.size() > 1; }
        size_t getQualifierCount() const { return qualifiers.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitQualifiedNameNode(this);
        }
    };
}
