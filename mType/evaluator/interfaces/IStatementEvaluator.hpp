#pragma once

#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>

namespace evaluator
{
    namespace interfaces
    {
        using namespace value;
        using ast::ASTNode;

        /**
         * @brief Interface for statement evaluation
         *
         * Provides abstraction for statement evaluators, enabling:
         * - Dependency Inversion: High-level modules depend on abstraction
         * - Testing: Mock implementations for unit testing
         * - Flexibility: Multiple implementations (interpreter, compiler, JIT, etc.)
         *
         * Design Principles:
         * - Interface Segregation: Minimal, focused interface
         * - Open/Closed: New implementations without modifying dependents
         * - Single Responsibility: Only statement evaluation
         */
        class IStatementEvaluator
        {
        public:
            virtual ~IStatementEvaluator() = default;

            /**
             * @brief Evaluate a statement AST node
             * @param node The statement node to evaluate
             * @return The value produced by the statement (if any)
             */
            virtual Value evaluate(ASTNode* node) = 0;

            /**
             * @brief Check if this evaluator can handle a given node
             * @param node The node to check
             * @return true if this evaluator can handle the node
             */
            virtual bool canHandle(ASTNode* node) const = 0;

            /**
             * @brief Convert a lambda value to an interface implementation
             * @param lambdaValue The lambda value to convert
             * @param interfaceName The name of the target interface
             * @param location Source location for error reporting
             * @return The converted value
             */
            virtual Value convertLambdaToInterface(
                const Value& lambdaValue,
                const std::string& interfaceName,
                const errors::SourceLocation& location = errors::SourceLocation()) = 0;
        };
    }
}
