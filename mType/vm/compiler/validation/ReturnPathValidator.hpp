#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ThrowNode.hpp"
#include "../../../ast/nodes/statements/SwitchNode.hpp"
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../errors/TypeException.hpp"
#include <string>

namespace vm::compiler::validation
{
    /**
     * Validates that non-void methods have return statements on all code paths
     */
    class ReturnPathValidator
    {
    public:
        /**
         * Validates that a method body has proper return statements
         * @param body The method body AST node
         * @param returnType The declared return type of the method
         * @param returnTypeStr The full return type string (e.g., "Promise<void>")
         * @param methodName The name of the method (for error messages)
         * @param location Source location for error reporting
         * @throws errors::TypeException if validation fails
         */
        static void validateMethodReturns(
            ast::ASTNode* body,
            value::ValueType returnType,
            const std::string& returnTypeStr,
            const std::string& methodName,
            const errors::SourceLocation& location
        );

        /**
         * Checks if a code path definitely returns or throws.
         * Used by guard-clause narrowing to detect early-exit patterns.
         * @param node The AST node to check
         * @return true if this path always returns or throws
         */
        static bool pathAlwaysReturns(ast::ASTNode* node);

        /**
         * MYT-381: Checks if a code path definitely exits the current loop
         * iteration: return, throw, continue, or (when breakExits) break.
         * breakExits must be false when the path is compiled inside a switch
         * context, because there `break` binds to the switch, not the loop.
         * Used only by guard-clause narrowing inside loops — NOT by
         * validateMethodReturns.
         * @param node The AST node to check
         * @param breakExits Whether a `break` counts as exiting the iteration
         * @return true if this path always exits the loop iteration
         */
        static bool pathAlwaysExitsLoopIteration(ast::ASTNode* node, bool breakExits);

    private:

        /**
         * Checks if a block always returns
         */
        static bool blockAlwaysReturns(ast::BlockNode* block);

        /**
         * Checks if an if statement always returns (both branches must return)
         */
        static bool ifAlwaysReturns(ast::IfNode* ifNode);

        /**
         * Checks if a switch statement always returns (all cases + default must return)
         */
        static bool switchAlwaysReturns(ast::SwitchNode* switchNode);

        /**
         * Checks if a try statement always returns (try, all catches, and finally must coordinate)
         */
        static bool tryAlwaysReturns(ast::TryNode* tryNode);

        /**
         * MYT-381: loop-iteration-exit counterparts of the *AlwaysReturns
         * helpers. Same recursion shape; leaf set additionally includes
         * continue and (when breakExits) break.
         */
        static bool blockAlwaysExits(ast::BlockNode* block, bool breakExits);

        static bool ifAlwaysExits(ast::IfNode* ifNode, bool breakExits);

        static bool switchAlwaysExits(ast::SwitchNode* switchNode);

        static bool tryAlwaysExits(ast::TryNode* tryNode, bool breakExits);
    };
}
