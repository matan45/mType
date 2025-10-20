#pragma once
#include "ExportRegistry.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>
#include <string>

namespace environment::registry
{
    /**
     * @brief Utility for collecting exported symbols from AST nodes
     *
     * This class provides shared functionality for both the evaluator and VM
     * to collect exported symbols (classes, interfaces, functions, variables)
     * from parsed AST trees.
     *
     * Following SOLID principles:
     * - Single Responsibility: Only handles export symbol collection
     * - Open/Closed: Extensible for new symbol types
     * - Dependency Inversion: Depends on ExportRegistry abstraction
     *
     * This eliminates code duplication between:
     * - ImportAndFunctionHandler (evaluator)
     * - BytecodeCompiler (VM)
     */
    class ExportSymbolCollector
    {
    public:
        /**
         * @brief Collect all exported symbols from an AST tree
         *
         * Traverses the AST and registers all exportable symbols
         * (classes, interfaces, functions, variables) with their
         * visibility modifiers.
         *
         * @param ast The root AST node (typically ProgramNode)
         * @param filePath The source file path for the symbols
         * @param exportRegistry The registry to store exported symbols
         */
        static void collectExportedSymbols(ASTNode* ast,
                                          const std::string& filePath,
                                          std::shared_ptr<ExportRegistry> exportRegistry);

        /**
         * @brief Collect exported symbols from a single AST node
         *
         * Examines a single node and registers it if it's an exportable symbol type.
         *
         * @param node The AST node to examine
         * @param filePath The source file path for the symbol
         * @param exportRegistry The registry to store the exported symbol
         */
        static void collectExportedSymbolsFromNode(ASTNode* node,
                                                   const std::string& filePath,
                                                   std::shared_ptr<ExportRegistry> exportRegistry);

        // Prevent instantiation
        ExportSymbolCollector() = delete;
        ~ExportSymbolCollector() = delete;
        ExportSymbolCollector(const ExportSymbolCollector&) = delete;
        ExportSymbolCollector& operator=(const ExportSymbolCollector&) = delete;
    };
}
