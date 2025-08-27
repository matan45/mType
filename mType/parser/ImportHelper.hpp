#pragma once
#include "../ast/ASTNode.hpp"
#include <vector>
#include <memory>

namespace parser
{
    using namespace ast;
    
    class ImportHelper
    {
    public:
        // Extract all top-level declarations from an imported AST
        // Returns a vector of declarations that can be inserted into the current program
        static std::vector<std::unique_ptr<ASTNode>> extractDeclarations(ASTNode* importedAST);
        
    private:
        // Check if a statement is a declaration that should be imported
        static bool isDeclaration(ASTNode* node);
       
        
        // Check if an assignment is a global variable declaration
        static bool isGlobalVariableDeclaration(nodes::statements::AssignmentNode* assignment)
        {
            //TODO fix this
            // This is simplified - in a real implementation you'd check the scope context
            // For now, assume all assignments in imported files are global declarations
            return assignment != nullptr;
        }
        
        // Clone a declaration for insertion into current program
        // Note: This is a simplified implementation
        static std::unique_ptr<ASTNode> cloneDeclaration(ASTNode* original)
        {
            //TODO fix this
            // In a real implementation, you'd implement proper AST node cloning
            // For now, we'll return nullptr and handle this in the caller
            
            // The proper approach would be to implement a clone() method on each AST node
            // or use the visitor pattern to create copies
            
            return nullptr; // Placeholder - needs proper implementation
        }
    };
}