#pragma once

#include <string>
#include <cstddef>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "../errors/SourceLocation.hpp"

namespace debugger
{
    // Use SourceLocation from errors namespace
    using errors::SourceLocation;

    // Forward declarations
    class CallFrame;
    struct DebugEvent;

    /**
     * Represents the current debugging mode
     */
    enum class DebugMode
    {
        DISABLED, // Normal execution (no debugging)
        STEP_INTO, // Step into function calls
        STEP_OVER, // Step over function calls (stop at same depth)
        STEP_OUT, // Step out of current function
        CONTINUE, // Continue until next breakpoint
        PAUSED // Currently paused at breakpoint
    };

    /**
     * Represents the current execution state
     */
    enum class ExecutionState
    {
        RUNNING, // Actively executing
        PAUSED, // Paused at breakpoint or step
        STOPPED, // Program terminated
        NOT_STARTED // Not yet started
    };

    /**
     * Represents a breakpoint location
     */
    struct BreakpointKey
    {
        std::string filename;
        int line;

        bool operator==(const BreakpointKey& other) const
        {
            return filename == other.filename && line == other.line;
        }
    };

    /**
     * Custom hash function for BreakpointKey
     */
    struct BreakpointKeyHash
    {
        std::size_t operator()(const BreakpointKey& key) const
        {
            return std::hash<std::string>()(key.filename) ^ (std::hash<int>()(key.line) << 1);
        }
    };

    /**
     * Represents breakpoint information (condition, log message, etc.)
     */
    struct BreakpointInfo
    {
        std::string condition; // Optional condition expression
        std::string logMessage; // Optional log message (for log points)
        int hitCount = 0; // Number of times this breakpoint was hit

        bool isLogPoint() const
        {
            return !logMessage.empty();
        }

        bool hasCondition() const
        {
            return !condition.empty();
        }
    };

    /**
     * Represents a call frame in the call stack
     */
    class CallFrame
    {
    public:
        std::string functionName;
        SourceLocation location;
        int depth;

        CallFrame(const std::string& name, const SourceLocation& loc, int d)
            : functionName(name), location(loc), depth(d)
        {
        }
    };

    /**
     * Represents a debug event
     */
    struct DebugEvent
    {
        enum class Type
        {
            BREAKPOINT_HIT,
            STEP_COMPLETE,
            EXCEPTION_THROWN,
            SCRIPT_STARTED,
            SCRIPT_COMPLETE
        };

        Type type;
        SourceLocation location;
        std::string message;

        DebugEvent(Type t, const SourceLocation& loc, const std::string& msg = "")
            : type(t), location(loc), message(msg)
        {
        }
    };

    /**
     * Main debug context that manages debug state
     *
     * This class follows the Singleton pattern to ensure global access
     * and coordinates all debugging operations including breakpoints,
     * stepping, and execution control.
     */
    class DebugContext
    {
    private:
        static std::unique_ptr<DebugContext> instance;
        static std::mutex instanceMutex;

        // Debug state
        DebugMode mode;
        ExecutionState state;
        std::atomic<bool> enabled;

        // Breakpoints (maps location to info including conditions and log messages)
        std::unordered_map<BreakpointKey, BreakpointInfo, BreakpointKeyHash> breakpoints;
        mutable std::mutex breakpointMutex;

        // Exception breakpoints
        std::unordered_set<std::string> exceptionBreakpoints; // e.g., "all", "uncaught"
        mutable std::mutex exceptionMutex;

        // Call stack
        std::vector<CallFrame> callStack;
        mutable std::mutex callStackMutex;

        // Stepping state
        int stepStartDepth;
        SourceLocation lastStopLocation;

        // Pause/resume mechanism
        std::mutex pauseMutex;
        std::condition_variable pauseCondition;
        std::atomic<bool> paused;

        // Event callback
        std::function<void(const DebugEvent&)> eventCallback;
        mutable std::mutex callbackMutex;

        // Condition evaluator callback (returns true if condition is met)
        std::function<bool(const std::string&)> conditionEvaluator;
        mutable std::mutex evaluatorMutex;

        // Private constructor for singleton
        DebugContext();

    public:
        // Singleton access
        static DebugContext& getInstance();
        static void initialize();
        static void shutdown();

        // Prevent copying
        DebugContext(const DebugContext&) = delete;
        DebugContext& operator=(const DebugContext&) = delete;

        // Enable/disable debugging
        void enable();
        void disable();
        bool isEnabled() const;

        // Breakpoint management
        void addBreakpoint(const std::string& filename, int line,
                           const std::string& condition = "",
                           const std::string& logMessage = "");
        void removeBreakpoint(const std::string& filename, int line);
        void clearBreakpoints(const std::string& filename);
        void clearAllBreakpoints();
        bool hasBreakpoint(const SourceLocation& location) const;
        bool hasBreakpoint(const std::string& filename, int line) const;
        BreakpointInfo* getBreakpointInfo(const std::string& filename, int line);
        std::vector<BreakpointKey> getBreakpoints() const;

        // Exception breakpoint management
        void setExceptionBreakpoints(const std::vector<std::string>& filters);
        void clearExceptionBreakpoints();
        bool shouldBreakOnException(bool isUncaught = false) const;

        // Execution control
        void pause();
        void resume();
        void stepInto();
        void stepOver();
        void stepOut();
        void continueExecution();
        void stop();

        // State queries
        DebugMode getMode() const;
        ExecutionState getState() const;
        bool isPaused() const;
        bool shouldPauseAt(const SourceLocation& location);

        // Call stack management
        void pushCallFrame(const std::string& functionName, const SourceLocation& location);
        void popCallFrame();
        std::vector<CallFrame> getCallStack() const;
        int getCurrentDepth() const;

        // Event handling
        void setEventCallback(std::function<void(const DebugEvent&)> callback);
        void notifyEvent(const DebugEvent& event);

        // Condition evaluation
        void setConditionEvaluator(std::function<bool(const std::string&)> evaluator);
        bool evaluateCondition(const std::string& condition);

        // Wait for resume (blocks until resumed)
        void waitForResume();

    private:
        bool shouldStopForStepping(const SourceLocation& location);
    };
} // namespace debugger
