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
         * Which leaf statements count as "exiting" the analyzed path. Lets the
         * single recursion below serve both callers: return/throw always exit;
         * pathAlwaysReturns uses {false,false}; pathAlwaysExitsLoopIteration
         * uses {continueExits=true, breakExits} so a new control-flow node only
         * has to be taught one tree.
         */
        struct ExitCriteria
        {
            bool continueExits;
            bool breakExits;
        };

        /**
         * Recursion shared by pathAlwaysReturns and
         * pathAlwaysExitsLoopIteration. return/throw always exit; continue and
         * break exit per `crit`.
         */
        static bool pathAlwaysExits(ast::ASTNode* node, ExitCriteria crit);

        static bool blockAlwaysExits(ast::BlockNode* block, ExitCriteria crit);

        static bool ifAlwaysExits(ast::IfNode* ifNode, ExitCriteria crit);

        static bool switchAlwaysExits(ast::SwitchNode* switchNode, ExitCriteria crit);

        static bool tryAlwaysExits(ast::TryNode* tryNode, ExitCriteria crit);
    };
}
