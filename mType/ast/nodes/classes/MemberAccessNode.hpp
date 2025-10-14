#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>

namespace ast::nodes::classes
{
    class MemberAccessNode : public ASTNode
    {
    private:
        std::shared_ptr<ASTNode> object;
        std::string memberName;
        bool isStaticAccess; // For Class::member vs obj.member

    public:
        // Constructor accepting shared_ptr
        explicit MemberAccessNode(std::shared_ptr<ASTNode> obj, const std::string& member,
                                  bool isStatic = false, const SourceLocation& loc = SourceLocation());

        // Constructor accepting unique_ptr for backward compatibility
        explicit MemberAccessNode(std::unique_ptr<ASTNode> obj, const std::string& member,
                                  bool isStatic = false, const SourceLocation& loc = SourceLocation());

        // For code that just needs to read
        [[nodiscard]] ASTNode* getObject() const noexcept;
        const std::string& getMemberName() const;
        bool getIsStaticAccess() const;

        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getObjectShared() const;

        // Safe transfer of ownership - returns shared_ptr that gets converted to unique_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> transferObjectOwnership() const;

        void setObject(std::shared_ptr<ASTNode> obj);
        void setMemberName(const std::string& member);
        void setIsStaticAccess(bool isStatic);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
