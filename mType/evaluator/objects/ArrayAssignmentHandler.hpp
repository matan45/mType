#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <vector>
#include <optional>

namespace evaluator {
    // Forward declaration
    class ExpressionEvaluator;
}

namespace evaluator {
namespace objects {

    using namespace base;
    using namespace value;
    using namespace errors;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles multi-dimensional array assignment operations
     *
     * Responsibilities:
     * - Index assignment evaluation
     * - Multi-dimensional array index extraction
     * - Direct multi-dimensional assignment for FlatMultiArray, SparseMultiArray, and NativeArray
     *
     * Design Principles:
     * - Single Responsibility: Only array assignment operations
     * - Supports: 2D arrays, 3D+ arrays, jagged arrays
     */
    class ArrayAssignmentHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        ExpressionEvaluator* exprEvaluator;

    public:
        explicit ArrayAssignmentHandler(std::shared_ptr<EvaluationContext> ctx);
        
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);

       
        Value evaluateIndexAssignment(IndexAssignmentNode* node);

    private:
        /**
         * Extract multi-dimensional assignment info (for 3D+ arrays)
         * Returns: optional pair of (base array, indices vector)
         */
        std::optional<std::pair<Value, std::vector<size_t>>> extractMultiDimensionalAssignment(
            IndexAssignmentNode* node);

        /**
         * Perform direct multi-dimensional assignment on the array
         */
        Value performDirectMultiDimensionalAssignment(
            const Value& baseArray,
            const std::vector<size_t>& indices,
            const Value& newValue,
            const SourceLocation& location);
    };

}
} 
