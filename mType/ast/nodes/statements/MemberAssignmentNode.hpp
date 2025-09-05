#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    class MemberAssignmentNode : public ASTNode
    {
    private:
        std::shared_ptr<ASTNode> object;
        std::string memberName;
        std::shared_ptr<ASTNode> value;

    public:
        // Constructor accepting shared_ptr
        explicit MemberAssignmentNode(std::shared_ptr<ASTNode> obj, const std::string& member, std::shared_ptr<ASTNode> val,
                             const SourceLocation& loc = SourceLocation());
        
        // Constructor accepting unique_ptr for backward compatibility
        explicit MemberAssignmentNode(std::unique_ptr<ASTNode> obj, const std::string& member, std::unique_ptr<ASTNode> val,
                             const SourceLocation& loc = SourceLocation());

        // For code that just needs to read
        [[nodiscard]] ASTNode* getObject() const noexcept;
        const std::string& getMemberName() const;
        [[nodiscard]] ASTNode* getValue() const noexcept;

        // Safe getters - return shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getObjectShared() const;
        [[nodiscard]] std::shared_ptr<ASTNode> getValueShared() const;

        void setObject(std::shared_ptr<ASTNode> obj);
        void setMemberName(const std::string& member);
        void setValue(std::shared_ptr<ASTNode> val);

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
