#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../parser/TypeParser.hpp"
#include <memory>
#include <vector>
#include <optional>

// Forward declarations for AST nodes
namespace ast {
namespace nodes {
namespace expressions {
    class ArrayCreationNode;
    class ArrayLiteralNode;
    class IndexAccessNode;
}
}
}

namespace evaluator {
    // Forward declarations
    class ExpressionEvaluator;
}

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::expressions;

    /**
     * @brief Handles array operations and access
     *
     * Responsibilities:
     * - Array creation (single and multi-dimensional)
     * - Array literal evaluation
     * - Array index access (single and multi-dimensional)
     * - Default value generation for array types
     *
     * Design Principles:
     * - Single Responsibility: Only array operations
     * - Supports NativeArray, FlatMultiArray, SparseMultiArray
     */
    class ArrayHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;

    public:
        explicit ArrayHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        /**
         * Evaluate array creation expression
         */
        Value evaluateArrayCreation(ArrayCreationNode* node);

        /**
         * Evaluate array literal expression
         */
        Value evaluateArrayLiteral(ArrayLiteralNode* node);

        /**
         * Evaluate array index access
         */
        Value evaluateIndexAccess(IndexAccessNode* node);

        /**
         * Get default value for a given type
         */
        Value getDefaultValueForType(const ::parser::TypeInfo& elementType);

        /**
         * Extract multi-dimensional array access pattern
         */
        std::optional<Value> extractMultiDimensionalAccess(
            IndexAccessNode* node, std::vector<size_t>& indices);

        /**
         * Evaluate direct multi-dimensional array access
         */
        Value evaluateDirectMultiDimensionalAccess(
            const Value& baseArray,
            const std::vector<size_t>& indices,
            const errors::SourceLocation& location);
    };

} // namespace expressions
} // namespace evaluator
