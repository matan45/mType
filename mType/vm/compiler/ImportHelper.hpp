#pragma once
#include "../../ast/ASTNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../environment/registry/ExportRegistry.hpp"
#include <memory>
#include <string>
#include <functional>

namespace vm::compiler
{
    /**
     * Helper class for import processing during compilation
     * Extracted from BytecodeCompiler to improve Single Responsibility Principle compliance
     * Handles: nested import processing, exported symbol collection
     */
    class ImportHelper
    {
    public:
        ImportHelper() = default;
        ~ImportHelper() = default;

        /**
         * Process nested imports recursively
         * @param node Root node to scan for imports
         * @param visitImportCallback Callback to handle each import node
         */
        void processNestedImports(ast::ASTNode* node,
                                 std::function<void(ast::ImportNode*)> visitImportCallback);

        /**
         * Collect exported symbols from an AST
         * @param ast Root AST node
         * @param filePath File path for symbol registration
         * @param exportRegistry Registry to store exported symbols
         */
        void collectExportedSymbols(ast::ASTNode* ast,
                                   const std::string& filePath,
                                   std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);

    private:
        /**
         * Collect exported symbols from a single node
         */
        void collectExportedSymbolsFromNode(ast::ASTNode* node,
                                           const std::string& filePath,
                                           std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);
    };
}
