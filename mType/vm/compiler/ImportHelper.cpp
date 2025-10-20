#include "ImportHelper.hpp"

namespace vm::compiler
{
    void ImportHelper::processNestedImports(ast::ASTNode* node,
                                           std::function<void(ast::ImportNode*)> visitImportCallback)
    {
        if (!node) return;

        // Handle ProgramNode - traverse all statements
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                processNestedImports(stmt.get(), visitImportCallback);
            }
        }
        // Handle ImportNode - process the import recursively
        else if (auto importNode = dynamic_cast<ast::ImportNode*>(node))
        {
            // Call the visitor callback for this import
            // This will recursively load all nested dependencies
            visitImportCallback(importNode);
        }
    }

    void ImportHelper::collectExportedSymbols(ast::ASTNode* ast,
                                             const std::string& filePath,
                                             std::shared_ptr<environment::registry::ExportRegistry> exportRegistry)
    {
        if (!ast) return;

        // Handle ProgramNode - traverse all statements
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(ast))
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

    void ImportHelper::collectExportedSymbolsFromNode(ast::ASTNode* node,
                                                      const std::string& filePath,
                                                      std::shared_ptr<environment::registry::ExportRegistry> exportRegistry)
    {
        using namespace ast;
        using namespace environment::registry;

        if (!node) return;

        // Register class
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                classNode->getClassName(),
                ExportedSymbolType::CLASS,
                classNode->getVisibility()
            );
        }
        // Register interface
        else if (auto interfaceNode = dynamic_cast<ast::InterfaceNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                interfaceNode->getName(),
                ExportedSymbolType::INTERFACE,
                interfaceNode->getVisibility()
            );
        }
        // Register function
        else if (auto functionNode = dynamic_cast<ast::FunctionNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                functionNode->getName(),
                ExportedSymbolType::FUNCTION,
                functionNode->getVisibility()
            );
        }
        // Register variable (top-level declarations)
        else if (auto assignmentNode = dynamic_cast<ast::AssignmentNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                assignmentNode->getVariableName(),
                ExportedSymbolType::VARIABLE,
                assignmentNode->getVisibility()
            );
        }
    }
}
