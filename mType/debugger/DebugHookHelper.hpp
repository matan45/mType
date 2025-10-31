#pragma once

#include "DebugContext.hpp"
#include "../ast/ASTNode.hpp"
#include <memory>

namespace debugger {

    /**
     * Helper class providing static methods to integrate debugging hooks
     * into the evaluator without tight coupling.
     *
     * This class follows the Helper/Utility pattern and provides convenient
     * methods for the evaluator to check for debug conditions and manage
     * debugging state during execution.
     */
    class DebugHookHelper {
    public:
        /**
         * Call before evaluating any AST node (statement level)
         *
         * This checks if debugger should pause at this location and handles
         * the pause/resume logic if necessary.
         *
         * @param node The AST node about to be evaluated
         * @return true if execution should pause, false otherwise
         */
        static bool preExecuteHook(ast::ASTNode* node);

        /**
         * Call after evaluating an AST node
         *
         * @param node The AST node that was just evaluated
         */
        static void postExecuteHook(ast::ASTNode* node);

        /**
         * Call when entering a function
         *
         * @param functionName Name of the function being called
         * @param location Source location of the function call
         */
        static void enterFunctionHook(
            const std::string& functionName,
            const SourceLocation& location
        );

        /**
         * Call when exiting a function
         *
         * @param functionName Name of the function that returned
         */
        static void exitFunctionHook(const std::string& functionName);

        /**
         * Check if debugging is currently enabled
         *
         * This is a fast check that can be used to avoid unnecessary work
         * when debugging is not active.
         *
         * @return true if debugging is enabled
         */
        static inline bool isDebuggingEnabled() {
            return DebugContext::getInstance().isEnabled();
        }

        /**
         * Check if should pause at the given location
         *
         * @param location The source location to check
         * @return true if execution should pause
         */
        static bool shouldPause(const SourceLocation& location);

        /**
         * Wait for debugger to resume execution
         *
         * This blocks the current thread until the debugger sends a continue
         * or step command.
         */
        static void waitForResume();

        /**
         * Notify debugger of script start
         *
         * @param scriptPath Path to the script being executed
         */
        static void notifyScriptStart(const std::string& scriptPath);

        /**
         * Notify debugger of script completion
         *
         * @param scriptPath Path to the script that completed
         */
        static void notifyScriptComplete(const std::string& scriptPath);

        /**
         * Notify debugger of exception
         *
         * @param exceptionType Type of exception
         * @param message Exception message
         * @param location Location where exception occurred
         */
        static void notifyException(
            const std::string& exceptionType,
            const std::string& message,
            const SourceLocation& location
        );

        /**
         * Check if should break on exception
         *
         * Call this when an exception is thrown to check if the debugger
         * wants to break on it. If true, the exception handler should pause
         * execution and wait for debugger commands.
         *
         * @param isUncaught Whether this exception is uncaught
         * @return true if should break on this exception
         */
        static bool shouldBreakOnException(bool isUncaught = false);

        /**
         * Handle exception breakpoint
         *
         * Call this when an exception is thrown. This will check if we should
         * break on this exception and handle the pause/notification if needed.
         *
         * @param exceptionType Type of exception
         * @param message Exception message
         * @param location Location where exception occurred
         * @param isUncaught Whether this exception is uncaught
         * @return true if execution was paused
         */
        static bool handleException(
            const std::string& exceptionType,
            const std::string& message,
            const SourceLocation& location,
            bool isUncaught = false
        );
    };

} // namespace debugger
