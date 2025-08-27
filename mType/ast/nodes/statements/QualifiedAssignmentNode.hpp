#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    class QualifiedAssignmentNode : public ASTNode
    {
    private:
        std::vector<std::string> qualifiedName; // e.g., ["namespace", "ClassName", "staticField"]
        std::unique_ptr<ASTNode> value;

    public:
        explicit QualifiedAssignmentNode(const std::vector<std::string>& qualName, std::unique_ptr<ASTNode> val,
                                const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), qualifiedName(qualName), value(std::move(val)) {}

        const std::vector<std::string>& getQualifiedName() const { return qualifiedName; }
        ASTNode* getValue() const { return value.get(); }
        std::string getSimpleName() const { return qualifiedName.empty() ? "" : qualifiedName.back(); }

        void setQualifiedName(const std::vector<std::string>& qualName) { qualifiedName = qualName; }
        void setValue(std::unique_ptr<ASTNode> val) { value = std::move(val); }

        bool isQualified() const { return qualifiedName.size() > 1; }
        size_t getQualifierCount() const { return qualifiedName.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitQualifiedAssignmentNode(this);
        }
    };
}
