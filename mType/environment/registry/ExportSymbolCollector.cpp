#include "ExportSymbolCollector.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"

namespace environment::registry
{
    void ExportSymbolCollector::collectExportedSymbols(ASTNode* ast,
                                                       const std::string& filePath,
                                                       std::shared_ptr<ExportRegistry> exportRegistry)
    {
        if (!ast) return;

        // Handle ProgramNode - traverse all statements
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(ast))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                collectExportedSymbolsFromNode(stmt.get(), filePath, exportRegistry);
            }
        }
        else
        {
            collectExportedSymbolsFromNode(ast, filePath, exportRegistry);
        }
    }

    void ExportSymbolCollector::collectExportedSymbolsFromNode(ASTNode* node,
                                                               const std::string& filePath,
                                                               std::shared_ptr<ExportRegistry> exportRegistry)
    {
        using namespace ast::nodes::classes;
        using namespace ast::nodes::functions;
        using namespace ast::nodes::statements;

        if (!node) return;

        // Register class
        if (auto classNode = dynamic_cast<ClassNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                classNode->getClassName(),
                ExportedSymbolType::CLASS,
                classNode->getVisibility()
            );
        }
        // Register interface
        else if (auto interfaceNode = dynamic_cast<InterfaceNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                interfaceNode->getName(),
                ExportedSymbolType::INTERFACE,
                interfaceNode->getVisibility()
            );
        }
        // Register function
        else if (auto functionNode = dynamic_cast<FunctionNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                functionNode->getName(),
                ExportedSymbolType::FUNCTION,
                functionNode->getVisibility()
            );
        }
        // Register variable (top-level declarations)
        else if (auto assignmentNode = dynamic_cast<AssignmentNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                assignmentNode->getVariableName(),
                ExportedSymbolType::VARIABLE,
                assignmentNode->getVisibility()
            );
        }
        // Handle other node types if needed
    }
}
