#include "TestUtilities.hpp"
#include <cstddef>
#include "DebugSession.hpp"
#include "ScriptAnalyzer.hpp"
#include "ErrorReporting.hpp"
#include "BenchmarkRunner.hpp"

#include "../diagnostics/DiagnosticRenderer.hpp"
#include "../diagnostics/TerminalDetect.hpp"

#include "../gc/GC.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"
#include "../version/Version.hpp"
#include "../project/ProjectConfigParser.hpp"
#include "../project/ProjectBuilder.hpp"
#include "../project/ProjectDiscovery.hpp"
#include "../project/WorkspaceConfigParser.hpp"
#include "../project/WorkspaceBuilder.hpp"
#include "../project/DependencyGraphBuilder.hpp"
#include "../project/DependencyGraphFormatter.hpp"
#include "../vm/profiler/ProfilerMode.hpp"
#include "../vm/profiler/ProfilerContext.hpp"
#include "../vm/profiler/ProfilerReport.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <exception>
#include <cstdlib>
#include <typeinfo>

namespace {
    // MYT-251: route std::terminate through our printer. The previous
    // attempt at the inlining real-fix exited with `0xE06D7363` (MSVC
    // C++ exception SEH magic) and zero output — neither the SEH filter
    // below nor the OSR catch handlers in OSRManager::executeOSRLoop
    // saw it because the C++ unwind path runs `std::terminate()` before
    // the OS surfaces the SEH event. This handler prints the typed
    // exception name + message before aborting so future failures
    // surface a usable diagnostic.
    [[noreturn]] void mtype_terminate_handler() noexcept
    {
        std::cerr.flush();
        std::cerr << "\nFATAL: std::terminate invoked"
#ifdef _WIN32
                  << " (likely C++ exception escape — exit code 0xE06D7363 family)"
#endif
                  << ".\n";
        if (auto eptr = std::current_exception())
        {
            try {
                std::rethrow_exception(eptr);
            }
            catch (const std::exception& e) {
                std::cerr << "  active exception: " << typeid(e).name()
                          << ": " << e.what() << "\n";
            }
            catch (...) {
                std::cerr << "  active exception: <non-std type, unknown>\n";
            }
        }
        else
        {
            std::cerr << "  no active exception object (called directly).\n";
        }
        std::cerr.flush();
        std::abort();
    }
}

#ifdef _WIN32
// MYT-248/249/250: SEH top-level filter so JIT-emitted code that triggers a
// structured exception (access violation from a stale IC pointer, JIT helper
// returning bad memory, etc.) prints a diagnostic instead of silently
// terminating. With MSVC's default /EHsc, catch(...) does NOT catch SEH —
// without this filter the process disappears with no output.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
namespace {
    LONG WINAPI mtype_seh_filter(EXCEPTION_POINTERS* ep)
    {
        std::cerr.flush();
        std::cerr << "\nFATAL: structured exception 0x"
                  << std::hex
                  << (ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0u)
                  << " at addr 0x"
                  << static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
                       ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr))
                  << std::dec
                  << " — likely JIT-emitted code dereferenced freed/invalid memory.\n";
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;  // Let the OS terminate normally
                                           // so post-mortem debuggers still attach.
    }

    // MYT-251: vectored exception handler. Fires BEFORE the normal SEH
    // dispatcher walk, which means it sees C++ exceptions (0xE06D7363)
    // even when the unwinder cannot traverse asmjit JIT-emitted frames
    // (no PE x64 unwind data registered for the JIT region) — the path
    // that bypasses both `set_terminate` and `SetUnhandledExceptionFilter`
    // and produces a silent exit on stream_pipeline_hot / reflection_lookup_hot.
    // Returns EXCEPTION_CONTINUE_SEARCH so normal handling still runs (this
    // is a diagnostic, not a handler). Gated by MTYPE_TRACE_VEH=1 so the
    // VEH does not interfere with normal program operation when not
    // diagnosing a silent crash.
    LONG WINAPI mtype_veh_diagnostic(EXCEPTION_POINTERS* ep)
    {
        if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
        const DWORD code = ep->ExceptionRecord->ExceptionCode;

        // Filter out the noise: ignore everything except first-chance
        // events that historically silently terminate. 0xE06D7363 is MSVC
        // C++ throw raise; 0xC0000005 is access violation; 0xC0000409 is
        // /GS cookie failure (__fastfail); 0xC0000374 is heap corruption.
        // Page faults during normal operation (0x80000003 single-step etc.)
        // are filtered out.
        const bool isInteresting =
            code == 0xE06D7363u || code == 0xC0000005u ||
            code == 0xC0000409u || code == 0xC0000374u ||
            code == 0xC00000FDu;  // stack overflow
        if (!isInteresting) return EXCEPTION_CONTINUE_SEARCH;

        std::cerr.flush();
        std::cerr << "[VEH] first-chance exception code=0x"
                  << std::hex << code
                  << " at addr=0x"
                  << static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
                       ep->ExceptionRecord->ExceptionAddress))
                  << std::dec;
        if (code == 0xE06D7363u && ep->ExceptionRecord->NumberParameters >= 3)
        {
            // Parameter[2] is a pointer to the ThrowInfo descriptor;
            // dereference cautiously inside __try / SEH not C++ try.
            std::cerr << " (C++ throw, ThrowInfo=0x"
                      << std::hex
                      << static_cast<std::uint64_t>(
                             ep->ExceptionRecord->ExceptionInformation[2])
                      << std::dec << ")";
        }
        std::cerr << "\n";
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;
    }
}
#endif

using namespace parser;
using namespace lexer;
using namespace services;
using namespace environment;

int main(int argc, char* argv[])
{
    // MYT-251: install the std::terminate handler before any user code runs
    // (cross-platform, complementary to the Windows SEH filter below). The
    // previous inlining attempt exited silently with 0xE06D7363; this is
    // the diagnostic that catches that path next time.
    std::set_terminate(mtype_terminate_handler);

#ifdef _WIN32
    // MYT-248/249/250: install before any JIT compilation runs so silent SEH
    // failures inside the JIT or its native helpers (the symptom of these
    // three bugs) at least print a diagnostic line.
    SetUnhandledExceptionFilter(mtype_seh_filter);

    // MYT-251: optional vectored exception handler — set MTYPE_TRACE_VEH=1
    // to install. Fires on first-chance exceptions before the normal
    // dispatch walk, so it sees throws even when the C++ unwind cannot
    // traverse asmjit JIT regions (no PE x64 unwind info registered).
    // This is the path that produces silent 0xE06D7363 exits on the
    // stream_pipeline_hot / reflection_lookup_hot benchmarks.
    if (const char* v = std::getenv("MTYPE_TRACE_VEH");
        v && v[0] == '1' && v[1] == '\0')
    {
        AddVectoredExceptionHandler(/*first=*/1, mtype_veh_diagnostic);
        std::cerr << "[VEH] installed vectored exception diagnostic handler\n";
        std::cerr.flush();
    }
#endif

    // Bytecode VM is the only execution mode
    constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM;

    // Pre-scan for color flags so the diagnostic renderer is configured
    // before any subcommand handler can throw and need to report. The
    // main flag-parsing loop further down also accepts these flags so
    // any duplicates harmlessly re-set the same mode.
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

    auto isTestArg = [](const std::string& s) {
        return s == "--tests";
    };
    auto isTestFlag = [](const std::string& s) {
        return s == "--no-jit" || s == "--tests" || s == "--test";
    };

    // Check for test suite execution. Find `--test <suite>` anywhere in argv
    // (so `--test integration --no-jit` and `--no-jit --test integration` both
    // work).
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (std::string(argv[i]) == "--test")
        {
            std::string suiteName = argv[i + 1];
            // Guard against `--test --no-jit` (no suite name).
            if (!isTestFlag(suiteName))
            {
                int failures = runSpecificTestSuite(suiteName, execMode, testJitEnabled);
                return failures == 0 ? 0 : 1;
            }
        }
    }

    // `--tests` anywhere in argv runs the full suite. Combined with --no-jit
    // for the JIT-disabled regression pass.
    for (int i = 1; i < argc; ++i)
    {
        if (isTestArg(argv[i]))
        {
            int failures = runAllTests(execMode, testJitEnabled);
            return failures == 0 ? 0 : 1;
        }
    }

    if (argc >= 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v"))
    {
        std::cout << "mType " << mType::version::getVersionString() << "\n";
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "mType " << mType::version::getVersionString() << "\n\n";
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <script_file.mt>           - Run a script file\n";
        std::cout << "  " << argv[0] << " --debug <script.mt>        - Run with debugger (breakpoints, stepping)\n";
        std::cout << "  " << argv[0] << " --gc-stats <script.mt>     - Run and print GC statistics after execution\n";
        std::cout << "  " << argv[0] << " --jit-stats <script.mt>    - Run and print JIT statistics after execution\n";
        std::cout << "  " << argv[0] << " --no-jit <script.mt>       - Disable JIT compilation (JIT is on by default)\n";
        std::cout << "  " << argv[0] << " --profile <script.mt>      - Run with profiler (light mode: function timing)\n";
        std::cout << "  " << argv[0] << " --profile=full <script.mt> - Run with full profiler (timing + call graph + opcodes)\n";
        std::cout << "  " << argv[0] << " --profile-output=json      - Output profiler report as JSON\n";
        std::cout << "  " << argv[0] << " --no-color                 - Disable colored diagnostic output (also: NO_COLOR env var)\n";
        std::cout << "  " << argv[0] << " --color=always|auto|never  - Force color mode (default: auto / TTY-detect)\n";
        std::cout << "  " << argv[0] << " --compile <script.mt>      - Compile to bytecode file (.mtc)\n";
        std::cout << "  " << argv[0] << " --run-cached <file.mtc>    - Run pre-compiled bytecode file\n";
        std::cout << "  " << argv[0] << " --build [project.mtproj]   - Build project (compile all files to bytecode)\n";
        std::cout << "  " << argv[0] << " --build --lib [.mtproj]    - Build project into single .mtcLib file\n";
        std::cout << "  " << argv[0] << " --build --exe [.mtproj]    - Build standalone executable with embedded bytecode\n";
        std::cout << "  " << argv[0] << " --clean [project.mtproj]   - Remove compiled bytecode files\n";
        std::cout << "  " << argv[0] << " --deps [project.mtproj]    - Show dependency tree\n";
        std::cout << "  " << argv[0] << " --deps --json [.mtproj]    - Export dependency graph as JSON\n";
        std::cout << "  " << argv[0] << " --deps --dot [.mtproj]     - Export dependency graph as Graphviz DOT\n";
        std::cout << "  " << argv[0] << " --deps --cycles [.mtproj]  - Detect circular dependencies\n";
        std::cout << "  " << argv[0] <<
            " --deps --why <file> [.mtproj] - Show import chain to a file\n";
        std::cout << "  " << argv[0] <<
            " --init <name> <include>    - Create new .mtproj file (e.g. --init MyApp src/**/*.mt)\n";
        std::cout << "  " << argv[0] <<
            " --init-workspace <name>    - Create new .mtworkspace file\n";
        std::cout << "  " << argv[0] << " --add <pattern> [.mtproj]  - Add include pattern to project\n";
        std::cout << "  " << argv[0] << " --remove <pattern> [.mtproj] - Remove include pattern from project\n";
        std::cout << "  " << argv[0] <<
            " --find-script-classes <script.mt> - Analyze script and show all @Script classes\n";
        std::cout << "  " << argv[0] <<
            " --test-script-objects <script.mt> - Demo: Create objects and call methods from C++\n";
        std::cout << "  " << argv[0] << " --tests                    - Run all test suites (JIT on)\n";
        std::cout << "  " << argv[0] << " --tests --no-jit           - Run all test suites with JIT disabled (regression pass)\n";
        std::cout << "  " << argv[0] << " --test <suite>             - Run specific test suite (JIT on)\n";
        std::cout << "  " << argv[0] << " --test <suite> --no-jit    - Run specific test suite with JIT disabled\n";
        std::cout << "  " << argv[0] << " --benchmark                - Run the interpreter benchmark suite\n";
        std::cout << "  " << argv[0] << " --benchmark=<script.mt>    - Run a single benchmark script\n";
        std::cout << "  " << argv[0] << " --benchmark-lexer=<path>   - Run a lexer-only microbenchmark on this .mt file\n";
        std::cout << "  " << argv[0] << " --benchmark-iterations=<N> - Measured iterations per script (default 3)\n";
        std::cout << "  " << argv[0] << " --benchmark-output=<fmt>   - Output format: text (default) or json\n";
        std::cout << "  " << argv[0] << " --help                     - Show this help message\n";
        std::cout << "  " << argv[0] << " --version, -v              - Print version (" << mType::version::getVersionString() << ") and exit\n\n";
        printAvailableTestSuites();
        return 0;
    }

    // Handle --build command (project/workspace compilation)
    if (argc >= 2 && std::string(argv[1]) == "--build")
    {
        std::string configPath;
        bool buildLib = false;
        bool buildExe = false;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--lib")
            {
                buildLib = true;
            }
            else if (arg == "--exe")
            {
                buildExe = true;
            }
            else if (arg[0] != '-')
            {
                configPath = arg;
            }
        }

        try
        {
            // Determine if we're building a workspace or a project
            bool isWorkspace = false;

            if (!configPath.empty())
            {
                // Explicit path: check extension
                if (configPath.size() > 12 &&
                    configPath.substr(configPath.size() - 12) == ".mtworkspace")
                {
                    isWorkspace = true;
                }
            }
            else
            {
                // Auto-detect: workspace takes priority over project
                auto discovery = project::findProjectOrWorkspace(".");
                if (!discovery)
                {
                    std::cerr << "Error: No .mtproj or .mtworkspace file found in current directory or parents\n";
                    return 1;
                }
                configPath = discovery->path;
                isWorkspace = (discovery->type == project::DiscoveryType::WORKSPACE);
            }

            if (isWorkspace)
            {
                // Workspace build
                if (buildLib)
                {
                    std::cerr << "Error: --lib is not supported for workspace builds\n";
                    return 1;
                }
                if (buildExe)
                {
                    std::cerr << "Error: --exe is not supported for workspace builds\n";
                    return 1;
                }

                project::WorkspaceConfigParser wsParser;
                std::cout << "Loading workspace: " << configPath << "\n";

                auto wsConfig = wsParser.parse(configPath);

                std::cout << "Workspace: " << wsConfig->name;
                if (!wsConfig->version.empty())
                {
                    std::cout << " v" << wsConfig->version;
                }
                std::cout << "\n";
                std::cout << "Member projects: " << wsConfig->members.size() << "\n";

                for (const auto& member : wsConfig->members)
                {
                    std::cout << "  - " << member.getName() << " (" << member.path << ")\n";
                }
                std::cout << "\n";

                project::WorkspaceBuilder builder;
                builder.setProgressCallback([](const project::WorkspaceBuildProgress& progress)
                {
                    std::cout << "[" << progress.projectName << " "
                              << progress.fileProgress.current << "/"
                              << progress.fileProgress.total << "] "
                              << progress.fileProgress.currentFile << "\n";
                });

                auto result = builder.build(*wsConfig);

                std::cout << "\nWorkspace build " << (result.success ? "succeeded" : "failed") << "\n";
                std::cout << "  Projects: " << result.projectsBuilt << " built";
                if (result.projectsFailed > 0)
                {
                    std::cout << ", " << result.projectsFailed << " failed";
                }
                std::cout << "\n";
                std::cout << "  Files:    " << result.totalFilesCompiled << " compiled";
                if (result.totalFilesFailed > 0)
                {
                    std::cout << ", " << result.totalFilesFailed << " failed";
                }
                std::cout << "\n";
                std::cout << "  Duration: " << result.duration.count() << "ms\n";

                if (!result.errors.empty())
                {
                    std::cout << "\nErrors:\n";
                    for (const auto& error : result.errors)
                    {
                        std::cout << "  " << error << "\n";
                    }
                }

                return result.success ? 0 : 1;
            }
            else
            {
                // Single project build
                project::ProjectConfigParser parser;
                std::cout << "Loading project: " << configPath << "\n";

                auto config = parser.parse(configPath);

                std::cout << "Project: " << config->name;
                if (!config->version.empty())
                {
                    std::cout << " v" << config->version;
                }
                std::cout << "\n";
                std::cout << "Source files: " << config->resolvedSourceFiles.size() << "\n";
                std::cout << "Output directory: " << config->output.directory << "\n";

                project::ProjectBuilder builder;

                builder.setProgressCallback([](const project::BuildProgress& progress)
                {
                    std::cout << "[" << progress.current << "/" << progress.total << "] " << progress.currentFile << "\n";
                });

                project::BuildResult result;

                if (buildLib)
                {
                    std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
                    std::string libPath = (outputDir / (config->name + ".mtcLib")).string();
                    std::cout << "Building library: " << libPath << "\n\n";
                    result = builder.buildLibrary(*config, libPath);
                }
                else if (buildExe)
                {
                    std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
#ifdef _WIN32
                    std::string exeName = config->name + ".exe";
#else
                    std::string exeName = config->name;
#endif
                    std::string exePath = (outputDir / exeName).string();

                    // Locate the launcher binary relative to the mType executable
                    std::filesystem::path mtypePath = std::filesystem::path(argv[0]).parent_path();
#ifdef _WIN32
                    std::string launcherPath = (mtypePath / "mtype-launcher.exe").string();
#else
                    std::string launcherPath = (mtypePath / "mtype-launcher").string();
#endif

                    std::cout << "Building executable: " << exePath << "\n\n";
                    result = builder.buildExecutable(*config, exePath, launcherPath);
                }
                else
                {
                    std::cout << "\n";
                    result = builder.build(*config);
                }

                std::cout << "\nBuild " << (result.success ? "succeeded" : "failed") << "\n";
                std::cout << "  Compiled: " << result.filesCompiled << " files\n";
                if (result.filesFailed > 0)
                {
                    std::cout << "  Failed:   " << result.filesFailed << " files\n";
                }
                std::cout << "  Duration: " << result.duration.count() << "ms\n";

                if (!result.errors.empty())
                {
                    std::cout << "\nErrors:\n";
                    for (const auto& error : result.errors)
                    {
                        std::cout << "  " << error << "\n";
                    }
                }

                return result.success ? 0 : 1;
            }
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --clean command
    if (argc >= 2 && std::string(argv[1]) == "--clean")
    {
        std::string configPath;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg[0] != '-')
            {
                configPath = arg;
                break;
            }
        }

        try
        {
            bool isWorkspace = false;

            if (!configPath.empty())
            {
                if (configPath.size() > 12 &&
                    configPath.substr(configPath.size() - 12) == ".mtworkspace")
                {
                    isWorkspace = true;
                }
            }
            else
            {
                auto discovery = project::findProjectOrWorkspace(".");
                if (!discovery)
                {
                    std::cerr << "Error: No .mtproj or .mtworkspace file found in current directory or parents\n";
                    return 1;
                }
                configPath = discovery->path;
                isWorkspace = (discovery->type == project::DiscoveryType::WORKSPACE);
            }

            if (isWorkspace)
            {
                project::WorkspaceConfigParser wsParser;
                auto wsConfig = wsParser.parse(configPath);

                project::WorkspaceBuilder builder;
                builder.clean(*wsConfig);

                std::cout << "Clean completed for workspace: " << wsConfig->name << "\n";
                for (const auto& member : wsConfig->members)
                {
                    if (member.config)
                    {
                        std::cout << "  - " << member.getName() << ": " << member.config->output.directory << "\n";
                    }
                }
            }
            else
            {
                project::ProjectConfigParser parser;
                auto config = parser.parse(configPath);

                project::ProjectBuilder builder;
                builder.clean(*config);

                std::cout << "Clean completed. Removed: " << config->output.directory << "\n";
            }

            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --deps command (dependency graph analysis)
    if (argc >= 2 && std::string(argv[1]) == "--deps")
    {
        try
        {
            // Parse flags
            std::string configPath;
            bool jsonOutput = false;
            bool dotOutput = false;
            bool showCycles = false;
            std::string whyFile;

            for (int i = 2; i < argc; ++i)
            {
                std::string arg = argv[i];
                if (arg == "--json") jsonOutput = true;
                else if (arg == "--dot") dotOutput = true;
                else if (arg == "--cycles") showCycles = true;
                else if (arg == "--why" && i + 1 < argc) whyFile = argv[++i];
                else if (arg[0] != '-') configPath = arg;
            }

            // Auto-detect project/workspace if not specified
            bool isWorkspace = false;
            if (configPath.empty())
            {
                auto discovery = project::findProjectOrWorkspace(".");
                if (!discovery)
                {
                    std::cerr << "Error: No .mtproj or .mtworkspace file found\n";
                    return 1;
                }
                configPath = discovery->path;
                isWorkspace = (discovery->type == project::DiscoveryType::WORKSPACE);
            }
            else
            {
                isWorkspace = configPath.size() > 12 &&
                             configPath.substr(configPath.size() - 12) == ".mtworkspace";
            }

            bool colorEnabled = diagnostics::TerminalDetect::isTerminal(stdout)
                             && !diagnostics::TerminalDetect::noColorRequested();
            if (colorEnabled)
            {
                diagnostics::TerminalDetect::enableVirtualTerminalProcessing(stdout);
            }

            // Build the dependency graph
            project::DependencyGraphBuilder builder;
            project::DependencyGraph graph = [&]()
            {
                if (isWorkspace)
                {
                    project::WorkspaceConfigParser wsParser;
                    auto wsConfig = wsParser.parse(configPath);
                    return builder.build(*wsConfig);
                }
                else
                {
                    project::ProjectConfigParser projParser;
                    auto projConfig = projParser.parse(configPath);
                    return builder.build(*projConfig);
                }
            }();

            // Dispatch to the appropriate output
            if (showCycles)
            {
                auto cycles = graph.findCycles();
                project::DependencyGraphFormatter::renderCycles(
                    cycles, graph.getProjectRoot(), std::cout, colorEnabled);
                return cycles.empty() ? 0 : 1;
            }

            if (!whyFile.empty())
            {
                project::DependencyGraphFormatter::renderWhy(
                    graph, whyFile, std::cout, colorEnabled);
                return 0;
            }

            if (jsonOutput)
            {
                auto json = project::DependencyGraphFormatter::toJson(graph);
                std::cout << json->toJsonString(true) << "\n";
                return 0;
            }

            if (dotOutput)
            {
                project::DependencyGraphFormatter::renderDot(graph, std::cout);
                return 0;
            }

            // Default: render tree
            project::DependencyGraphFormatter::renderTree(
                graph, std::cout, colorEnabled);
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --init command (create new .mtproj file)
    if (argc >= 4 && std::string(argv[1]) == "--init")
    {
        std::string projectName = argv[2];
        std::string includePath = argv[3];

        std::string filename = projectName + ".mtproj";

        if (std::filesystem::exists(filename))
        {
            std::cerr << "Error: " << filename << " already exists\n";
            return 1;
        }

        std::ofstream outFile(filename);
        if (!outFile)
        {
            std::cerr << "Error: Could not create " << filename << "\n";
            return 1;
        }

        outFile << "<Project Name=\"" << projectName << "\" Version=\"1.0.0\">\n";
        outFile << "  <Source>\n";
        outFile << "    <Include>" << includePath << "</Include>\n";
        outFile << "  </Source>\n";
        outFile << "  <Output Directory=\"build\" />\n";
        outFile << "  <Imports>\n";
        outFile << "  </Imports>\n";
        outFile << "</Project>\n";

        outFile.close();

        std::cout << "Created " << filename << "\n";
        return 0;
    }

    // Handle --init-workspace command (create new .mtworkspace file)
    if (argc >= 3 && std::string(argv[1]) == "--init-workspace")
    {
        std::string workspaceName = argv[2];
        std::string filename = workspaceName + ".mtworkspace";

        if (std::filesystem::exists(filename))
        {
            std::cerr << "Error: " << filename << " already exists\n";
            return 1;
        }

        std::ofstream outFile(filename);
        if (!outFile)
        {
            std::cerr << "Error: Could not create " << filename << "\n";
            return 1;
        }

        outFile << "<Workspace Name=\"" << workspaceName << "\" Version=\"1.0.0\">\n";
        outFile << "  <Members>\n";

        // Add any member paths provided as extra arguments
        for (int i = 3; i < argc; ++i)
        {
            outFile << "    <Member Path=\"" << argv[i] << "\" />\n";
        }

        outFile << "  </Members>\n";
        outFile << "  <Output Directory=\"build\" />\n";
        outFile << "</Workspace>\n";

        outFile.close();

        std::cout << "Created " << filename << "\n";
        return 0;
    }

    // Handle --add command (add include pattern to project)
    if (argc >= 3 && std::string(argv[1]) == "--add")
    {
        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find </Source> and insert before it
            std::string searchTag = "</Source>";
            size_t pos = content.find(searchTag);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Invalid .mtproj format (missing </Source>)\n";
                return 1;
            }

            std::string newInclude = "    <Include>" + pattern + "</Include>\n  ";
            content.insert(pos, newInclude);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Added include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --remove command (remove include pattern from project)
    if (argc >= 3 && std::string(argv[1]) == "--remove")
    {
        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find and remove the include line
            std::string searchPattern = "<Include>" + pattern + "</Include>";
            size_t pos = content.find(searchPattern);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Pattern not found in project: " << pattern << "\n";
                return 1;
            }

            // Find the start of the line (go back to newline or start)
            size_t lineStart = pos;
            while (lineStart > 0 && content[lineStart - 1] != '\n')
            {
                --lineStart;
            }

            // Find end of line
            size_t lineEnd = pos + searchPattern.length();
            while (lineEnd < content.length() && content[lineEnd] != '\n')
            {
                ++lineEnd;
            }
            if (lineEnd < content.length())
            {
                ++lineEnd; // Include the newline
            }

            content.erase(lineStart, lineEnd - lineStart);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Removed include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --compile command
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--compile")
        {
            std::string sourceFile;

            // Parse remaining arguments for filename
            for (int j = i + 1; j < argc; ++j)
            {
                std::string arg = argv[j];
                if (arg[0] != '-')
                {
                    sourceFile = arg;
                }
            }

            if (sourceFile.empty())
            {
                std::cerr << "Error: No source file specified for --compile\n";
                return 1;
            }

            std::string outputFile = sourceFile + "c"; // .mt -> .mtc

            try
            {
                std::cout << "Compiling " << sourceFile << " to " << outputFile << "...\n";

                ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);
                interpreter.compileToFile(sourceFile, outputFile);
                return 0;
            }
            catch (const std::exception& e)
            {
                runMain::reportException(e);
                return 1;
            }
        }
    }

    // Handle --run-cached command
    if (argc == 3 && std::string(argv[1]) == "--run-cached")
    {
        std::string cachedFile = argv[2];

        try
        {
            std::cout << "Loading cached bytecode from " << cachedFile << "...\n";

            ScriptInterpreter interpreter;
            interpreter.getVM()->setJitEnabled(true);
            interpreter.runCompiledBytecode(cachedFile);
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --test-script-objects command
    if (argc >= 3 && std::string(argv[1]) == "--test-script-objects")
    {
        std::string scriptFile = argv[2];

        try
        {
            demonstrateScriptObjectUsage(scriptFile, constants::ExecutionMode::BYTECODE_VM);
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --find-script-classes command
    if (argc >= 3 && std::string(argv[1]) == "--find-script-classes")
    {
        std::string scriptFile = argv[2];

        try
        {
            std::cout << "Analyzing script: " << scriptFile << "\n";
            std::cout << "(Parsing and registering classes for analysis)\n\n";

            ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);

            // Parse and register classes without executing the script
            interpreter.parseAndRegisterClasses(scriptFile);

            // Query and print all @Script annotated classes
            auto environment = interpreter.getEnvironment();
            printScriptAnnotatedClasses(environment);

            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Parse debug mode, gc-stats flag, jit flag, profiler flags, and filename
    std::string filename;
    bool debugMode = false;
    bool printGCStats = false;
    bool printJitStats = false;
    bool enableJit = true;
    vm::profiler::ProfilerMode profileMode = vm::profiler::ProfilerMode::DISABLED;
    vm::profiler::ProfilerOutputFormat profileOutputFormat = vm::profiler::ProfilerOutputFormat::CONSOLE;

    bool benchmarkMode = false;
    runMain::BenchmarkOptions benchmarkOptions;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--debug")
        {
            debugMode = true;
        }
        else if (arg == "--gc-stats")
        {
            printGCStats = true;
        }
        else if (arg == "--jit-stats")
        {
            printJitStats = true;
        }
        else if (arg == "--no-jit")
        {
            enableJit = false;
        }
        else if (arg == "--profile" || arg == "--profile=light")
        {
            profileMode = vm::profiler::ProfilerMode::LIGHT;
        }
        else if (arg == "--profile=full")
        {
            profileMode = vm::profiler::ProfilerMode::FULL;
        }
        else if (arg == "--profile-output=json")
        {
            profileOutputFormat = vm::profiler::ProfilerOutputFormat::JSON;
        }
        else if (arg == "--benchmark")
        {
            benchmarkMode = true;
        }
        else if (arg.rfind("--benchmark=", 0) == 0)
        {
            benchmarkMode = true;
            benchmarkOptions.singleScript = arg.substr(std::string("--benchmark=").size());
        }
        else if (arg.rfind("--benchmark-lexer=", 0) == 0)
        {
            benchmarkMode = true;
            benchmarkOptions.singleLexerScript = arg.substr(std::string("--benchmark-lexer=").size());
        }
        else if (arg.rfind("--benchmark-iterations=", 0) == 0)
        {
            benchmarkMode = true;
            try
            {
                int n = std::stoi(arg.substr(std::string("--benchmark-iterations=").size()));
                if (n > 0) benchmarkOptions.measuredIterations = n;
            }
            catch (...)
            {
                std::cerr << "Warning: invalid --benchmark-iterations value, using default\n";
            }
        }
        else if (arg.rfind("--benchmark-output=", 0) == 0)
        {
            benchmarkMode = true;
            std::string fmt = arg.substr(std::string("--benchmark-output=").size());
            if (fmt == "json")
            {
                benchmarkOptions.outputFormat = runMain::BenchmarkOutput::Json;
            }
            else if (fmt == "text")
            {
                benchmarkOptions.outputFormat = runMain::BenchmarkOutput::Text;
            }
            else
            {
                std::cerr << "Warning: --benchmark-output=" << fmt
                          << " not recognized (use text|json), defaulting to text\n";
            }
        }
        else if (arg == "--no-color" || arg == "--color=never")
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
        else if (arg[0] != '-')
        {
            filename = arg;
        }
    }

    if (benchmarkMode)
    {
        benchmarkOptions.jitEnabled = enableJit;
        benchmarkOptions.printJitStats = printJitStats;
        // MYT-251 (Phase C1): runBenchmarks runs OUTSIDE the existing
        // script-execution try/catch below, so any C++ exception escaping
        // the benchmark loop went straight to std::set_terminate (or
        // bypassed everything via the asmjit unwind hole). Wrap and
        // rethrow so the typed exception name surfaces at the actual
        // escape point — the existing handlers still produce their
        // secondary diagnostic if rethrow propagates.
        try
        {
            int rc = runMain::runBenchmarks(benchmarkOptions);
            gc::GC::shutdown();
            reflection::ReflectionNatives::cleanup();
            json::JsonNatives::cleanup();
            return rc;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[bench-fatal] std::exception escaped runBenchmarks: "
                      << typeid(e).name() << ": " << e.what() << "\n";
            std::cerr.flush();
            throw;
        }
        catch (...)
        {
            std::cerr << "[bench-fatal] unknown C++ exception escaped runBenchmarks "
                      << "— likely thrown from JIT-emitted code or a JIT helper "
                      << "whose call site lacked a C++ unwind frame.\n";
            std::cerr.flush();
            throw;
        }
    }

    if (filename.empty())
    {
        std::cerr << "Error: No script file specified\n";
        std::cerr << "Use --help for usage information\n";
        return 1;
    }

    // Run in debug mode if --debug flag present
    if (debugMode)
    {
        runInDebugMode(filename, execMode);
        return 0;
    }

    try
    {
        // Initialize profiler if requested
        if (profileMode != vm::profiler::ProfilerMode::DISABLED)
        {
            vm::profiler::ProfilerContext::initialize(profileMode, profileOutputFormat);
        }

        ScriptInterpreter interpreter(execMode);

        if (enableJit)
        {
            interpreter.getVM()->setJitEnabled(true);
            std::cout << "Execution Mode: Bytecode VM + JIT\n\n";
        }
        else
        {
            std::cout << "Execution Mode: Bytecode VM\n\n";
        }
        // MYT-248/249/250: flush so the mode header is visible even if the
        // script terminates abnormally (silent SEH crash / unhandled non-std
        // exception). std::cout is line-buffered to a TTY but not always to
        // pipes, and the post-loop print() never reaches us in those cases.
        std::cout.flush();

        interpreter.runScript(filename);

        // MYT-35 follow-up — drain any non-fatal compile-time warnings
        // (unused variables, missing @Override, etc.) and render them
        // through the same Rust-style renderer used for errors. Done
        // after runScript so the user sees output first, then warnings.
        runMain::reportWarnings(interpreter.getCompilerWarnings());

        // Force a GC collection at program end to detect any remaining cycles
        gc::GC::forceCollect();

        // Print GC statistics if requested
        if (printGCStats)
        {
            gc::GC::printStats();
        }

        // Print JIT statistics if requested
        if (printJitStats)
        {
            interpreter.getVM()->printJitStats();
        }

        // Generate profiler report if profiling was enabled
        if (profileMode != vm::profiler::ProfilerMode::DISABLED)
        {
            auto& profilerCtx = vm::profiler::ProfilerContext::getInstance();
            profilerCtx.finalize();
            vm::profiler::ProfilerReport::generate(profilerCtx);
            vm::profiler::ProfilerContext::shutdown();
        }
    }
    catch (const std::exception& e)
    {
        runMain::reportException(e);
        // Clean up profiler before exit
        if (profileMode != vm::profiler::ProfilerMode::DISABLED)
        {
            vm::profiler::ProfilerContext::shutdown();
        }
        gc::GC::shutdown(); // Clean up GC before exit
        reflection::ReflectionNatives::cleanup();
        json::JsonNatives::cleanup();
        return 1;
    }
    catch (...)
    {
        // MYT-248/249/250: silent zero-output failures under JIT were
        // diagnosed as exceptions escaping runScript that derive from
        // neither std::exception nor errors::UserException — without this
        // catch the CRT terminates the process with no message and no
        // crash dialog in Release. Always emit a diagnostic so the failure
        // mode is at least visible.
        std::cerr << "Error: unhandled non-std exception escaped script execution "
                  << "(JIT-emitted code or native helper). "
                  << "Check JIT IC / OSR / helper-call path." << std::endl;
        if (profileMode != vm::profiler::ProfilerMode::DISABLED)
        {
            vm::profiler::ProfilerContext::shutdown();
        }
        gc::GC::shutdown();
        reflection::ReflectionNatives::cleanup();
        json::JsonNatives::cleanup();
        return 2;
    }

    // MYT-251: post-main cleanup runs OUTSIDE the main try/catch. Prior
    // failures along the inlining path produced exit code 0xE06D7363
    // (MSVC C++ exception) with NO diagnostic output — the throw escaped
    // here. Wrap so any cleanup-time exception surfaces a typed name
    // before the process tears down.
    try
    {
        gc::GC::shutdown();
        reflection::ReflectionNatives::cleanup();
        json::JsonNatives::cleanup();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[cleanup] caught std::exception during post-main shutdown: "
                  << typeid(e).name() << ": " << e.what() << "\n";
        std::cerr.flush();
        return 3;
    }
    catch (...)
    {
        std::cerr << "[cleanup] caught unknown (non-std) exception during "
                  << "post-main shutdown.\n";
        std::cerr.flush();
        return 4;
    }

    return 0;
}
