#include "MainExceptionHandlers.hpp"
#include "ErrorReporting.hpp"
#include "commands/MainCommands.hpp"

#include "../constants/ExecutionMode.hpp"
#include "../diagnostics/DiagnosticRenderer.hpp"

#include <string>

int main(int argc, char* argv[])
{
    runMain::installCrashHandlers();

    // Bytecode VM is the only execution mode
    constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM;

    // Pre-scan for color flags so the diagnostic renderer is configured
    // before any subcommand handler can throw and need to report. The
    // executeDefaultScript flag-parsing loop further down also accepts
    // these flags so any duplicates harmlessly re-set the same mode.
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--no-color" || arg == "--color=never")
        {
            runMain::setColorMode(diagnostics::DiagnosticRenderer::ColorMode::Never);
        }
        else if (arg == "--color=always")
        {
            runMain::setColorMode(diagnostics::DiagnosticRenderer::ColorMode::Always);
        }
        else if (arg == "--color=auto")
        {
            runMain::setColorMode(diagnostics::DiagnosticRenderer::ColorMode::Auto);
        }
    }

    // MYT-259: scan for --no-jit anywhere in argv so it can be combined with
    // --tests / --test (e.g. `mType --tests --no-jit` or `mType --no-jit --tests`).
    // Default: JIT is on.
    bool testJitEnabled = true;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--no-jit")
        {
            testJitEnabled = false;
            break;
        }
    }

    using namespace runMain::commands;

    // Test dispatch first: `--test`/`--tests` may appear anywhere in argv
    // (combined with `--no-jit`), so it must run before strict argv[1]==X
    // handlers.
    if (auto r = handleTestCommand(argc, argv, execMode, testJitEnabled); r) return *r;

    // argv[1]==X handlers. handleInitWorkspaceCommand comes before handleInitCommand
    // even though the current checks use `==` (not prefix-match) — preserves
    // the original ordering for safety against future loosening.
    if (auto r = handleVersionCommand(argc, argv);            r) return *r;
    if (auto r = handleHelpCommand(argc, argv);               r) return *r;
    if (auto r = handleBuildCommand(argc, argv);              r) return *r;
    if (auto r = handleCleanCommand(argc, argv);              r) return *r;
    if (auto r = handleDepsCommand(argc, argv);               r) return *r;
    if (auto r = handleInitWorkspaceCommand(argc, argv);      r) return *r;
    if (auto r = handleInitCommand(argc, argv);               r) return *r;
    if (auto r = handleAddCommand(argc, argv);                r) return *r;
    if (auto r = handleRemoveCommand(argc, argv);             r) return *r;
    if (auto r = handleRunCachedCommand(argc, argv);          r) return *r;
    if (auto r = handleTestScriptObjectsCommand(argc, argv);  r) return *r;
    if (auto r = handleFindScriptClassesCommand(argc, argv);  r) return *r;

    // `--compile` scans argv for the flag (may appear in any position);
    // run after the strict argv[1] handlers so it doesn't shadow them.
    if (auto r = handleCompileCommand(argc, argv);            r) return *r;

    return executeDefaultScript(argc, argv, execMode);
}
