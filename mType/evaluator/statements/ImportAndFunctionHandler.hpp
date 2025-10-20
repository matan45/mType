#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>

// Forward declarations
namespace ast {
    class ASTNode;
namespace nodes {
namespace statements {
    class ImportNode;
}
namespace functions {
    class FunctionNode;
}
}
}

namespace environment {
namespace registry {
    class ExportRegistry;
}
}

namespace evaluator {
namespace statements {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;

    /**
     * @brief Handles import and function registration
     *
     * Responsibilities:
     * - Import statement processing
     * - Function definition registration
     * - Native function registration
     * - Lambda-to-interface conversion
     *
     * Design Principles:
     * - Single Responsibility: Only imports and function registration
     * - Delegates to ImportManager for actual import logic
     * - Dependency Inversion: Depends on interfaces
     */
    class ImportAndFunctionHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        interfaces::IExpressionEvaluator* exprEvaluator;
        interfaces::IStatementEvaluator* stmtEvaluator;

    public:
        explicit ImportAndFunctionHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Evaluate an import statement
         */
        Value evaluateImport(ImportNode* node);

        /**
         * Evaluate a function definition
         */
        Value evaluateFunction(FunctionNode* node);

    private:
        /**
         * Collect all exported symbols from an AST (imported file)
         * Traverses the AST and registers all classes, interfaces, and functions
         * in the ExportRegistry with their visibility modifiers.
         */
        void collectExportedSymbols(ast::ASTNode* ast,
                                   const std::string& filePath,
                                   std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);

        /**
         * Collect exported symbols from a single AST node
         * Helper method for collectExportedSymbols that processes individual nodes.
         */
        void collectExportedSymbolsFromNode(ast::ASTNode* node,
                                           const std::string& filePath,
                                           std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);

        /**
         * Validate and import symbols from a selective or wildcard import
         * For selective imports, validates that each requested symbol exists and is public.
         * For wildcard imports, no validation is needed (all public symbols are imported).
         */
        void validateAndImportSymbols(ImportNode* node,
                                     const std::string& resolvedPath,
                                     std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);

        /**
         * Get a formatted string of available public symbols from a file
         * Used for error messages to show what symbols are available when an import fails.
         * Returns "(none)" if no public symbols exist, or a comma-separated list of symbol names.
         */
        std::string getAvailableSymbolsString(const std::string& filePath,
                                             std::shared_ptr<environment::registry::ExportRegistry> exportRegistry);
    };

} // namespace statements
} // namespace evaluator
