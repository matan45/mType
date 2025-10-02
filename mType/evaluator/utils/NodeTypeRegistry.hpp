#pragma once
#include "../../ast/ASTNode.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include <typeindex>
#include <unordered_set>

namespace evaluator::utils
{
    using namespace ast;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;

    /**
     * @brief Centralized node type categorization following DRY principle
     * Eliminates duplicate type-checking logic across evaluator classes
     * Uses O(1) type_index lookups instead of O(n) dynamic_cast chains
     */
    class NodeTypeRegistry
    {
    public:
        enum class NodeCategory
        {
            EXPRESSION,
            STATEMENT,
            OBJECT,
            UNKNOWN
        };

        /**
         * @brief Categorize a node into one of the main categories
         * @param node The AST node to categorize
         * @return NodeCategory enum
         */
        static NodeCategory categorize(ASTNode* node);

        /**
         * @brief Check if node is an expression node
         * @param node The AST node to check
         * @return true if node is an expression, false otherwise
         */
        static bool isExpression(ASTNode* node);

        /**
         * @brief Check if node is a statement node
         * @param node The AST node to check
         * @return true if node is a statement, false otherwise
         */
        static bool isStatement(ASTNode* node);

        /**
         * @brief Check if node is an object-related node
         * @param node The AST node to check
         * @return true if node is object-related, false otherwise
         */
        static bool isObject(ASTNode* node);

    private:
        // Static type registries for O(1) lookups
        static const std::unordered_set<std::type_index> expressionTypes;
        static const std::unordered_set<std::type_index> statementTypes;
        static const std::unordered_set<std::type_index> objectTypes;

        // Initialization helper
        static std::unordered_set<std::type_index> initExpressionTypes();
        static std::unordered_set<std::type_index> initStatementTypes();
        static std::unordered_set<std::type_index> initObjectTypes();

        // Static utility class - prevent instantiation
        NodeTypeRegistry() = delete;
        ~NodeTypeRegistry() = delete;
        NodeTypeRegistry(const NodeTypeRegistry&) = delete;
        NodeTypeRegistry& operator=(const NodeTypeRegistry&) = delete;
    };
}
