#pragma once
#include "../ast/ASTNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"
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
        static std::vector<std::unique_ptr<ASTNode>> extractDeclarations(ASTNode* importedAST)
        {
            std::vector<std::unique_ptr<ASTNode>> declarations;
            
            if (!importedAST) {
                return declarations;
            }
            
            // Cast to ProgramNode (imported files are parsed as programs)
            if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(importedAST)) {
                const auto& statements = programNode->getStatements();
                
                for (const auto& stmt : statements) {
                    if (isDeclaration(stmt.get())) {
                        // Clone the declaration for insertion into current program
                        // Note: This requires implementing a clone method or copy constructor
                        // For now, we'll create a placeholder approach
                        declarations.push_back(cloneDeclaration(stmt.get()));
                    }
                }
            }
            
            return declarations;
        }
        
    private:
        // Check if a statement is a declaration that should be imported
        static bool isDeclaration(ASTNode* node)
        {
            // Import these types of declarations:
            // - Function definitions
            // - Global variable declarations
            // - Class definitions
            // - Namespace definitions
            // - Native function declarations
            
            return dynamic_cast<ast::nodes::functions::FunctionNode*>(node) != nullptr ||
                   dynamic_cast<ast::nodes::classes::ClassNode*>(node) != nullptr ||
                   dynamic_cast<ast::nodes::namespaces::NamespaceNode*>(node) != nullptr ||
                   (dynamic_cast<ast::nodes::statements::AssignmentNode*>(node) != nullptr &&
                    isGlobalVariableDeclaration(dynamic_cast<ast::nodes::statements::AssignmentNode*>(node)));
        }
        
        // Check if an assignment is a global variable declaration
        static bool isGlobalVariableDeclaration(ast::nodes::statements::AssignmentNode* assignment)
        {
            // This is simplified - in a real implementation you'd check the scope context
            // For now, assume all assignments in imported files are global declarations
            return assignment != nullptr;
        }
        
        // Clone a declaration for insertion into current program
        // Note: This is a simplified implementation
        static std::unique_ptr<ASTNode> cloneDeclaration(ASTNode* original)
        {
            // In a real implementation, you'd implement proper AST node cloning
            // For now, we'll return nullptr and handle this in the caller
            
            // The proper approach would be to implement a clone() method on each AST node
            // or use the visitor pattern to create copies
            
            return nullptr; // Placeholder - needs proper implementation
        }
    };
}