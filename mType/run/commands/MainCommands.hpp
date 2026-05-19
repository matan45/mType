#pragma once

#include "../../constants/ExecutionMode.hpp"
#include <optional>

namespace runMain::commands
{
    // Convention: each handler returns std::optional<int>.
    //   std::nullopt → did not match argv; continue to next handler.
    //   int          → matched and ran; main() exits with this status.
    // executeDefaultScript is terminal — always returns an exit code.

    std::optional<int> handleVersionCommand(int argc, char* argv[]);
    std::optional<int> handleHelpCommand(int argc, char* argv[]);

    // Covers both `--tests` and `--test <suite>`. `testJitEnabled` is the
    // pre-scanned `--no-jit` value (MYT-259: `--no-jit` may appear in any
    // position relative to the test selector).
    std::optional<int> handleTestCommand(int argc, char* argv[],
                                         constants::ExecutionMode execMode,
                                         bool testJitEnabled);

    std::optional<int> handleBuildCommand(int argc, char* argv[]);
    std::optional<int> handleCleanCommand(int argc, char* argv[]);
    std::optional<int> handleDepsCommand(int argc, char* argv[]);

    std::optional<int> handleInitCommand(int argc, char* argv[]);
    std::optional<int> handleInitWorkspaceCommand(int argc, char* argv[]);
    std::optional<int> handleAddCommand(int argc, char* argv[]);
    std::optional<int> handleRemoveCommand(int argc, char* argv[]);

    std::optional<int> handleCompileCommand(int argc, char* argv[]);
    std::optional<int> handleRunCachedCommand(int argc, char* argv[]);
    std::optional<int> handleTestScriptObjectsCommand(int argc, char* argv[]);
    std::optional<int> handleFindScriptClassesCommand(int argc, char* argv[]);

    // Terminal: parses execution flags, dispatches debug / benchmark /
    // script-execution path, and unconditionally tears down GC + reflection
    // + profiler via GcRuntimeGuard on every exit path.
    int executeDefaultScript(int argc, char* argv[],
                             constants::ExecutionMode execMode);

    // RAII for the GC + reflection + (conditional) profiler shutdown triad
    // that today appears four times in Main.cpp. Caller invokes
    // `releaseAndShutdown()` to propagate cleanup-time failure exit codes
    // (3 for std::exception, 4 for non-std); the destructor is a safety
    // net only.
    class GcRuntimeGuard
    {
    public:
        GcRuntimeGuard() = default;
        GcRuntimeGuard(const GcRuntimeGuard&) = delete;
        GcRuntimeGuard& operator=(const GcRuntimeGuard&) = delete;
        ~GcRuntimeGuard();

        // Mark profiler as owned so releaseAndShutdown() / dtor will also
        // shut it down. Caller must have already called
        // vm::profiler::ProfilerContext::initialize().
        void enableProfiler() { profilerOwned_ = true; }

        // Returns 0 on clean shutdown, 3 on std::exception during shutdown,
        // 4 on non-std exception. Idempotent; subsequent calls (and the
        // dtor) no-op once released.
        int releaseAndShutdown();

    private:
        bool profilerOwned_ = false;
        bool released_ = false;
    };
}
