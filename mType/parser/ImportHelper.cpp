#include "ImportHelper.hpp"

#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"

namespace parser
{
    std::vector<std::unique_ptr<ASTNode>> ImportHelper::extractDeclarations(ASTNode* importedAST)
    {
        std::vector<std::unique_ptr<ASTNode>> declarations;
            
        if (!importedAST) {
            return declarations;
        }
            
        // Cast to ProgramNode (imported files are parsed as programs)
        if (auto programNode = dynamic_cast<nodes::statements::ProgramNode*>(importedAST)) {
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

    bool ImportHelper::isDeclaration(ASTNode* node)
    {
        // Import these types of declarations:
        // - Function definitions
        // - Global variable declarations
        // - Class definitions
        // - Namespace definitions
        // - Native function declarations
            
        return dynamic_cast<nodes::functions::FunctionNode*>(node) != nullptr ||
               dynamic_cast<nodes::classes::ClassNode*>(node) != nullptr ||
               dynamic_cast<nodes::namespaces::NamespaceNode*>(node) != nullptr ||
               (dynamic_cast<nodes::statements::AssignmentNode*>(node) != nullptr &&
                isGlobalVariableDeclaration(dynamic_cast<nodes::statements::AssignmentNode*>(node)));
    }
}
