#include "DebugContext.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>

namespace debugger {

    // Cache for current working directory (initialized once)
    static std::string cachedCwd;
    static std::once_flag cwdInitFlag;

    // Helper function to normalize file paths for comparison
    // PERFORMANCE: Optimized to minimize filesystem I/O operations
    static std::string normalizePath(const std::string& path) {
        // Initialize CWD cache once (avoids repeated filesystem calls)
        std::call_once(cwdInitFlag, []() {
            try {
                cachedCwd = std::filesystem::current_path().string();
            } catch (...) {
                cachedCwd = ".";
            }
        });

        try {
            std::filesystem::path p(path);

            // Make absolute if relative (using cached CWD, no filesystem I/O)
            if (p.is_relative()) {
                p = std::filesystem::path(cachedCwd) / p;
            }

            // Use lexically_normal() instead of weakly_canonical()
            // This avoids filesystem access for path normalization
            std::string normalized = p.lexically_normal().string();

            // Convert to lowercase for case-insensitive comparison on Windows
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

            // Replace backslashes with forward slashes for consistency
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            return normalized;
        } catch (...) {
            // If path normalization fails, just return lowercase version
            std::string result = path;
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);
            std::replace(result.begin(), result.end(), '\\', '/');
            return result;
        }
    }

    // Static member initialization
    std::unique_ptr<DebugContext> DebugContext::instance = nullptr;
    std::mutex DebugContext::instanceMutex;

    DebugContext::DebugContext()
        : mode(DebugMode::DISABLED),
          state(ExecutionState::NOT_STARTED),
          enabled(false),
          stepStartDepth(0),
          paused(false) {
    }

    DebugContext& DebugContext::getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = std::unique_ptr<DebugContext>(new DebugContext());
        }
        return *instance;
    }

    void DebugContext::initialize() {
        getInstance().enable();
    }

    void DebugContext::shutdown() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance) {
            instance->disable();
            instance.reset();
        }
    }

    void DebugContext::enable() {
        enabled = true;
        state = ExecutionState::PAUSED;
        mode = DebugMode::PAUSED;
    }

    void DebugContext::disable() {
        enabled = false;
        state = ExecutionState::STOPPED;
        mode = DebugMode::DISABLED;
    }

    bool DebugContext::isEnabled() const {
        return enabled.load();
    }

    void DebugContext::addBreakpoint(const std::string& filename, int line,
                                      const std::string& condition,
                                      const std::string& logMessage) {
        std::lock_guard<std::mutex> lock(breakpointMutex);

        // PERFORMANCE: Normalize path once at insertion time instead of on every lookup
        std::string normalizedFilename = normalizePath(filename);

        BreakpointInfo info;
        info.condition = condition;
        info.logMessage = logMessage;
        info.hitCount = 0;

        // Store with normalized path for O(1) lookup
        breakpoints[{normalizedFilename, line}] = info;
    }

    void DebugContext::removeBreakpoint(const std::string& filename, int line) {
        std::lock_guard<std::mutex> lock(breakpointMutex);

        // Normalize path for consistent lookup
        std::string normalizedFilename = normalizePath(filename);
        breakpoints.erase({normalizedFilename, line});
    }

    void DebugContext::clearBreakpoints(const std::string& filename) {
        std::lock_guard<std::mutex> lock(breakpointMutex);

        // Normalize path once for comparison (stored paths are already normalized)
        std::string normalizedFilename = normalizePath(filename);

        auto it = breakpoints.begin();
        while (it != breakpoints.end()) {
            // Compare against already-normalized stored path
            if (it->first.filename == normalizedFilename) {
                it = breakpoints.erase(it);
            } else {
                ++it;
            }
        }
    }

    void DebugContext::clearAllBreakpoints() {
        std::lock_guard<std::mutex> lock(breakpointMutex);
        breakpoints.clear();
    }

    bool DebugContext::hasBreakpoint(const SourceLocation& location) const {
        return hasBreakpoint(location.getFilename(), location.getLine());
    }

    bool DebugContext::hasBreakpoint(const std::string& filename, int line) const {
        std::lock_guard<std::mutex> lock(breakpointMutex);

        // PERFORMANCE: Normalize once and use O(1) hash lookup instead of O(n) iteration
        // This eliminates repeated normalizePath() calls on every breakpoint
        std::string normalizedFilename = normalizePath(filename);

        // Direct O(1) hash lookup - stored keys already have normalized paths
        return breakpoints.find({normalizedFilename, line}) != breakpoints.end();
    }

    BreakpointInfo* DebugContext::getBreakpointInfo(const std::string& filename, int line) {
        std::lock_guard<std::mutex> lock(breakpointMutex);

        // PERFORMANCE: Normalize once and use O(1) hash lookup instead of O(n) iteration
        std::string normalizedFilename = normalizePath(filename);

        // Direct O(1) hash lookup - stored keys already have normalized paths
        auto it = breakpoints.find({normalizedFilename, line});
        return (it != breakpoints.end()) ? &it->second : nullptr;
    }

    std::vector<BreakpointKey> DebugContext::getBreakpoints() const {
        std::lock_guard<std::mutex> lock(breakpointMutex);
        std::vector<BreakpointKey> keys;
        for (const auto& [key, info] : breakpoints) {
            keys.push_back(key);
        }
        return keys;
    }

    void DebugContext::setExceptionBreakpoints(const std::vector<std::string>& filters) {
        std::lock_guard<std::mutex> lock(exceptionMutex);
        exceptionBreakpoints.clear();
        for (const auto& filter : filters) {
            exceptionBreakpoints.insert(filter);
        }
    }

    void DebugContext::clearExceptionBreakpoints() {
        std::lock_guard<std::mutex> lock(exceptionMutex);
        exceptionBreakpoints.clear();
    }

    bool DebugContext::shouldBreakOnException(bool isUncaught) const {
        std::lock_guard<std::mutex> lock(exceptionMutex);

        // Check if we should break on all exceptions
        if (exceptionBreakpoints.find("all") != exceptionBreakpoints.end()) {
            return true;
        }

        // Check if we should break on uncaught exceptions only
        if (isUncaught && exceptionBreakpoints.find("uncaught") != exceptionBreakpoints.end()) {
            return true;
        }

        return false;
    }

    void DebugContext::pause() {
        std::lock_guard<std::mutex> lock(pauseMutex);
        paused = true;
        state = ExecutionState::PAUSED;
        mode = DebugMode::PAUSED;
    }

    void DebugContext::resume() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            paused = false;
            state = ExecutionState::RUNNING;
        }
        pauseCondition.notify_one();
    }

    void DebugContext::stepInto() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            mode = DebugMode::STEP_INTO;
            stepStartDepth = getCurrentDepth();
            paused = false;
            state = ExecutionState::RUNNING;
        }
        pauseCondition.notify_one();
    }

    void DebugContext::stepOver() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            mode = DebugMode::STEP_OVER;
            stepStartDepth = getCurrentDepth();
            paused = false;
            state = ExecutionState::RUNNING;
        }
        pauseCondition.notify_one();
    }

    void DebugContext::stepOut() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            mode = DebugMode::STEP_OUT;
            stepStartDepth = getCurrentDepth();
            paused = false;
            state = ExecutionState::RUNNING;
        }
        pauseCondition.notify_one();
    }

    void DebugContext::continueExecution() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            mode = DebugMode::CONTINUE;
            paused = false;
            state = ExecutionState::RUNNING;
        }
        pauseCondition.notify_one();
    }

    void DebugContext::stop() {
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            state = ExecutionState::STOPPED;
            mode = DebugMode::DISABLED;
            paused = false;
        }
        pauseCondition.notify_all();
    }

    DebugMode DebugContext::getMode() const {
        return mode;
    }

    ExecutionState DebugContext::getState() const {
        return state;
    }

    bool DebugContext::isPaused() const {
        return paused.load();
    }

    bool DebugContext::shouldPauseAt(const SourceLocation& location) {
        if (!enabled || state == ExecutionState::STOPPED) {
            return false;
        }

        // Skip internal/generated code (has no source file)
        bool isInternalCode = location.getFilename().empty() ||
                              location.getFilename() == "<unknown>" ||
                              location.getLine() <= 0;

        // Don't pause at the same location twice in a row
        if (location.getFilename() == lastStopLocation.getFilename() &&
            location.getLine() == lastStopLocation.getLine()) {
            return false;
        }

        // When stepping (STEP_INTO, STEP_OVER, STEP_OUT), ignore breakpoints and only check stepping logic
        bool isStepping = (mode == DebugMode::STEP_INTO || mode == DebugMode::STEP_OVER || mode == DebugMode::STEP_OUT);

        if (isStepping) {
            // Check for stepping (but skip internal code)
            if (!isInternalCode && shouldStopForStepping(location)) {
                lastStopLocation = location;
                pause();
                notifyEvent(DebugEvent(DebugEvent::Type::STEP_COMPLETE, location));
                return true;
            }
            return false;
        }

        // In CONTINUE mode, check for breakpoints
        bool hasBp = hasBreakpoint(location);

        if (hasBp) {
            BreakpointInfo* bpInfo = getBreakpointInfo(location.getFilename(), location.getLine());
            if (bpInfo) {
                bpInfo->hitCount++;

                // Check if this is a log point
                if (bpInfo->isLogPoint()) {
                    // Log points don't pause execution, just output
                    notifyEvent(DebugEvent(
                        DebugEvent::Type::SCRIPT_STARTED,  // Reuse this for log output
                        location,
                        "LogPoint: " + bpInfo->logMessage
                    ));
                    return false;  // Don't pause
                }

                // Check condition if present
                if (bpInfo->hasCondition()) {
                    bool conditionMet = evaluateCondition(bpInfo->condition);
                    if (!conditionMet) {
                        return false;  // Condition not met, don't pause
                    }
                }

                // Condition met or no condition, pause
                lastStopLocation = location;
                pause();
                notifyEvent(DebugEvent(DebugEvent::Type::BREAKPOINT_HIT, location));
                return true;
            }
        }

        return false;
    }

    bool DebugContext::shouldStopForStepping(const SourceLocation& location) {
        int currentDepth = getCurrentDepth();

        switch (mode) {
            case DebugMode::STEP_INTO:
                // Stop at every statement (caller already filters out internal code)
                return true;

            case DebugMode::STEP_OVER:
                // Stop when we return to the same or shallower depth
                return currentDepth <= stepStartDepth;

            case DebugMode::STEP_OUT:
                // Stop when we return to a shallower depth
                return currentDepth < stepStartDepth;

            case DebugMode::CONTINUE:
            case DebugMode::PAUSED:
            case DebugMode::DISABLED:
                return false;
        }

        return false;
    }

    void DebugContext::pushCallFrame(const std::string& functionName, const SourceLocation& location) {
        std::lock_guard<std::mutex> lock(callStackMutex);
        int depth = static_cast<int>(callStack.size());
        callStack.emplace_back(functionName, location, depth);
    }

    void DebugContext::popCallFrame() {
        std::lock_guard<std::mutex> lock(callStackMutex);
        if (!callStack.empty()) {
            callStack.pop_back();
        }
    }

    std::vector<CallFrame> DebugContext::getCallStack() const {
        std::lock_guard<std::mutex> lock(callStackMutex);
        return callStack;
    }

    int DebugContext::getCurrentDepth() const {
        std::lock_guard<std::mutex> lock(callStackMutex);
        return static_cast<int>(callStack.size());
    }

    void DebugContext::setEventCallback(std::function<void(const DebugEvent&)> callback) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        eventCallback = callback;
    }

    void DebugContext::notifyEvent(const DebugEvent& event) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (eventCallback) {
            eventCallback(event);
        }
    }

    void DebugContext::waitForResume() {
        std::unique_lock<std::mutex> lock(pauseMutex);
        pauseCondition.wait(lock, [this] {
            return !paused.load() || state == ExecutionState::STOPPED;
        });
    }

    void DebugContext::setConditionEvaluator(std::function<bool(const std::string&)> evaluator) {
        std::lock_guard<std::mutex> lock(evaluatorMutex);
        conditionEvaluator = evaluator;
    }

    bool DebugContext::evaluateCondition(const std::string& condition) {
        std::lock_guard<std::mutex> lock(evaluatorMutex);

        if (!conditionEvaluator) {
            // No evaluator set, treat as always true (fail-safe behavior)
            return true;
        }

        try {
            return conditionEvaluator(condition);
        } catch (...) {
            // If evaluation fails, default to true (pause anyway)
            return true;
        }
    }

} // namespace debugger
