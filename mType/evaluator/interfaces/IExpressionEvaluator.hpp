#pragma once

#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include <string>

namespace evaluator
{
    namespace interfaces
    {
        using namespace value;
        using ast::ASTNode;

        /**
         * @brief Interface for expression evaluation
         *
         * Provides abstraction for expression evaluators, enabling:
         * - Dependency Inversion: High-level modules depend on abstraction
         * - Testing: Mock implementations for unit testing
         * - Flexibility: Multiple implementations (interpreter, compiler, etc.)
         *
         * Design Principles:
         * - Interface Segregation: Focused interface for expression evaluation
         * - Open/Closed: New implementations without modifying dependents
         * - Liskov Substitution: All implementations must honor contract
         *
         * Note: Includes conversion methods that handlers commonly need.
         */
        class IExpressionEvaluator
        {
        public:
            virtual ~IExpressionEvaluator() = default;

            /**
             * @brief Evaluate an expression AST node
             * @param node The expression node to evaluate
             * @return The evaluated value
             */
            virtual Value evaluate(ASTNode* node) = 0;

            /**
             * @brief Check if this evaluator can handle a given node
             * @param node The node to check
             * @return true if this evaluator can handle the node
             */
            virtual bool canHandle(ASTNode* node) const = 0;

            /**
             * @brief Convert a value to boolean (truthiness)
             * @param value The value to check
             * @return true if value is truthy
             */
            virtual bool isTruthy(const Value& value) const = 0;

            /**
             * @brief Convert a value to string representation
             * @param value The value to convert
             * @return String representation
             */
            virtual std::string toString(const Value& value) const = 0;

            /**
             * @brief Convert a value to float
             * @param value The value to convert
             * @return Float representation
             */
            virtual float toFloat(const Value& value) const = 0;

            /**
             * @brief Convert a value to integer
             * @param value The value to convert
             * @return Integer representation
             */
            virtual int toInt(const Value& value) const = 0;
        };
    }
}
