#pragma once
#include "../value/ValueType.hpp"
#include "../ast/ASTNode.hpp"

namespace services
{
    /**
     * Strategy interface for different execution modes
     * Implements the Strategy Pattern for AST/Bytecode/Dual execution
     */
    class ExecutionStrategy
    {
    public:
        virtual ~ExecutionStrategy() = default;

        /**
         * Execute the given AST node and return the result
         * @param ast The AST node to execute
         * @return The execution result value
         */
        virtual value::Value execute(ast::ASTNode* ast) = 0;
    };
}
