#include "DebugHookHelper.hpp"
#include  <iostream>
namespace debugger {

    bool DebugHookHelper::preExecuteHook(ast::ASTNode* node) {
        if (!isDebuggingEnabled()) {
            return false;
        }

        if (!node) {
            return false;
        }

        const SourceLocation& location = node->getLocation();

        // Only check for valid source locations
        if (location.getFilename().empty() || location.getLine() <= 0) {
            return false;
        }

        // Debug: log every statement being executed
        std::cerr << "Executing: " << location.getFilename() << ":" << location.getLine() << "\n";

        // Check if we should pause at this location
        DebugContext& debugCtx = DebugContext::getInstance();
        if (debugCtx.shouldPauseAt(location)) {
            std::cerr << "Paused at: " << location.getFilename() << ":" << location.getLine() << "\n";
            // Block until debugger resumes
            debugCtx.waitForResume();
            return true;
        }

        return false;
    }

    void DebugHookHelper::postExecuteHook(ast::ASTNode* node) {
        // Currently no post-execution logic needed
        // Can be extended in the future for profiling, tracing, etc.
    }

    void DebugHookHelper::enterFunctionHook(
        const std::string& functionName,
        const SourceLocation& location) {

        if (!isDebuggingEnabled()) {
            return;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.pushCallFrame(functionName, location);
    }

    void DebugHookHelper::exitFunctionHook(const std::string& functionName) {
        if (!isDebuggingEnabled()) {
            return;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.popCallFrame();
    }

    bool DebugHookHelper::shouldPause(const SourceLocation& location) {
        if (!isDebuggingEnabled()) {
            return false;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        return debugCtx.shouldPauseAt(location);
    }

    void DebugHookHelper::waitForResume() {
        if (!isDebuggingEnabled()) {
            return;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.waitForResume();
    }

    void DebugHookHelper::notifyScriptStart(const std::string& scriptPath) {
        if (!isDebuggingEnabled()) {
            return;
        }

        SourceLocation loc(scriptPath, 1, 1);

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.notifyEvent(DebugEvent(
            DebugEvent::Type::SCRIPT_STARTED,
            loc,
            "Script started: " + scriptPath
        ));
    }

    void DebugHookHelper::notifyScriptComplete(const std::string& scriptPath) {
        if (!isDebuggingEnabled()) {
            return;
        }

        SourceLocation loc(scriptPath, 0, 0);

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.notifyEvent(DebugEvent(
            DebugEvent::Type::SCRIPT_COMPLETE,
            loc,
            "Script completed: " + scriptPath
        ));
    }

    void DebugHookHelper::notifyException(
        const std::string& exceptionType,
        const std::string& message,
        const SourceLocation& location) {

        if (!isDebuggingEnabled()) {
            return;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        debugCtx.notifyEvent(DebugEvent(
            DebugEvent::Type::EXCEPTION_THROWN,
            location,
            exceptionType + ": " + message
        ));
    }

    bool DebugHookHelper::shouldBreakOnException(bool isUncaught) {
        if (!isDebuggingEnabled()) {
            return false;
        }

        DebugContext& debugCtx = DebugContext::getInstance();
        return debugCtx.shouldBreakOnException(isUncaught);
    }

    bool DebugHookHelper::handleException(
        const std::string& exceptionType,
        const std::string& message,
        const SourceLocation& location,
        bool isUncaught) {

        if (!isDebuggingEnabled()) {
            return false;
        }

        DebugContext& debugCtx = DebugContext::getInstance();

        // Check if we should break on this exception
        if (!debugCtx.shouldBreakOnException(isUncaught)) {
            return false;
        }

        // Notify debugger of exception
        notifyException(exceptionType, message, location);

        // Pause execution
        debugCtx.pause();
        debugCtx.waitForResume();

        return true;
    }

} // namespace debugger
