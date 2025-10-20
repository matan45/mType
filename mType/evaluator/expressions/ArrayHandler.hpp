#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../parser/TypeParser.hpp"
#include <memory>
#include <vector>
#include <optional>

// Forward declarations for AST nodes
namespace ast
{
    namespace nodes
    {
        namespace expressions
        {
            class ArrayCreationNode;
            class ArrayLiteralNode;
            class IndexAccessNode;
        }
    }
}

namespace evaluator
{
    namespace expressions
    {
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
         * - Dependency Inversion: Depends on IExpressionEvaluator interface
         */
        class ArrayHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            interfaces::IExpressionEvaluator* exprEvaluator;

        public:
            explicit ArrayHandler(std::shared_ptr<EvaluationContext> ctx);


            void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);

            Value evaluateArrayCreation(ArrayCreationNode* node);

            Value evaluateArrayLiteral(ArrayLiteralNode* node);

            Value evaluateIndexAccess(IndexAccessNode* node);

            Value getDefaultValueForType(const ::parser::TypeInfo& elementType);

            std::optional<Value> extractMultiDimensionalAccess(
                IndexAccessNode* node, std::vector<size_t>& indices);

            Value evaluateDirectMultiDimensionalAccess(
                const Value& baseArray,
                const std::vector<size_t>& indices,
                const SourceLocation& location);

        private:
            // Helper methods for evaluateArrayCreation refactoring
            std::string resolveElementClassName(
                const ::parser::TypeInfo& elementTypeInfo);

            Value create1DArrayWithDefaults(
                size_t size,
                const ::parser::TypeInfo& elementTypeInfo,
                const Value& defaultValue,
                const SourceLocation& location);

            Value createJaggedArrayWithNulls(
                const std::vector<size_t>& dimensions,
                const ::parser::TypeInfo& elementTypeInfo,
                const SourceLocation& location);

            Value createAndValidateMultiDimensionalArray(
                const std::vector<size_t>& dimensions,
                const ::parser::TypeInfo& elementTypeInfo,
                const Value& defaultValue,
                const SourceLocation& location);

            void validateFlatMultiArray(
                const std::shared_ptr<FlatMultiArray>& flatArray,
                const std::vector<size_t>& dimensions,
                const SourceLocation& location);

            void validateSparseMultiArray(
                const std::shared_ptr<SparseMultiArray>& sparseArray,
                const std::vector<size_t>& dimensions,
                const SourceLocation& location);

            void validateFlatMultiObjectArray(
                const std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>& flatMultiObjectArray,
                const std::vector<size_t>& dimensions,
                const SourceLocation& location);

            // Helper methods for evaluateIndexAccess refactoring
            Value handleNativeArrayAccess(
                const std::shared_ptr<value::NativeArray>& nativeArray,
                int index,
                const SourceLocation& location);

            Value handleFlatMultiArrayAccess(
                const std::shared_ptr<value::FlatMultiArray>& flatArray,
                int index,
                const SourceLocation& location);

            Value handleSparseMultiArrayAccess(
                const std::shared_ptr<value::SparseMultiArray>& sparseArray,
                int index,
                const SourceLocation& location);

            Value handleFlatMultiObjectArrayAccess(
                const std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>& flatMultiObjectArray,
                int index,
                const SourceLocation& location);
        };
    }
}
