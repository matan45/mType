#pragma once
#include "../../ASTNode.hpp"
#include <string>

namespace ast::nodes::statements
{
    // Wrapper node that represents an imported declaration
    // This preserves the import context while making the declaration available
    class ImportedDeclarationNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> originalDeclaration;  // Reference to the original declaration (owned by ImportManager)
        std::string sourceFile;        // File where this declaration came from
        
    public:
        explicit ImportedDeclarationNode(std::unique_ptr<ASTNode> decl, const std::string& file, 
                                       const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), originalDeclaration(std::move(decl)), sourceFile(file) {}
            
        ASTNode* getOriginalDeclaration() const { return originalDeclaration.get(); }
        const std::string& getSourceFile() const { return sourceFile; }
        
        // Delegate visitor pattern to the original declaration
        Value accept(ASTVisitor<Value>& visitor) override {
            // The evaluator should treat this as if it were the original declaration
            return originalDeclaration->accept(visitor);
        }
    };
}