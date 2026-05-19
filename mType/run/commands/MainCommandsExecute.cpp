#include "MainCommands.hpp"

#include "../BenchmarkRunner.hpp"
#include "../DebugSession.hpp"
#include "../ErrorReporting.hpp"
#include "../../diagnostics/DiagnosticRenderer.hpp"
#include "../../gc/GC.hpp"
#include "../../json/JsonNatives.hpp"
#include "../../reflection/ReflectionNatives.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../vm/profiler/ProfilerContext.hpp"
#include "../../vm/profiler/ProfilerMode.hpp"
#include "../../vm/profiler/ProfilerReport.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

namespace runMain::commands
{
    GcRuntimeGuard::~GcRuntimeGuard()
    {
        if (!released_) releaseAndShutdown();
    }

    int GcRuntimeGuard::releaseAndShutdown()
    {
        if (released_) return 0;
        released_ = true;

        // MYT-251: post-main cleanup runs OUTSIDE the main try/catch in the
        // original Main.cpp. Failures along the inlining path produced exit
        // code 0xE06D7363 (MSVC C++ exception) with NO diagnostic output —
        // the throw escaped here. Wrap so any cleanup-time exception
        // surfaces a typed name before the process tears down.
        try
        {
            if (profilerOwned_)
            {
                vm::profiler::ProfilerContext::shutdown();
            }
            gc::GC::shutdown();
            reflection::ReflectionNatives::cleanup();
            json::JsonNatives::cleanup();
            return 0;
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
    }

    namespace
    {
        struct ExecutionFlags
        {
            std::string filename;
            bool debugMode = false;
            bool printGCStats = false;
            bool printJitStats = false;
            bool enableJit = true;
            vm::profiler::ProfilerMode profileMode = vm::profiler::ProfilerMode::DISABLED;
            vm::profiler::ProfilerOutputFormat profileOutputFormat = vm::profiler::ProfilerOutputFormat::CONSOLE;
            bool benchmarkMode = false;
            runMain::BenchmarkOptions benchmarkOptions;
        };

        ExecutionFlags parseExecutionFlags(int argc, char* argv[])
        {
            ExecutionFlags flags;

            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];

                if (arg == "--debug")
                {
                    flags.debugMode = true;
                }
                else if (arg == "--gc-stats")
                {
                    flags.printGCStats = true;
                }
                else if (arg == "--jit-stats")
                {
                    flags.printJitStats = true;
                }
                else if (arg == "--no-jit")
                {
                    flags.enableJit = false;
                }
                else if (arg == "--profile" || arg == "--profile=light")
                {
                    flags.profileMode = vm::profiler::ProfilerMode::LIGHT;
                }
                else if (arg == "--profile=full")
                {
                    flags.profileMode = vm::profiler::ProfilerMode::FULL;
                }
                else if (arg == "--profile-output=json")
                {
                    flags.profileOutputFormat = vm::profiler::ProfilerOutputFormat::JSON;
                }
                else if (arg == "--benchmark")
                {
                    flags.benchmarkMode = true;
                }
                else if (arg.rfind("--benchmark=", 0) == 0)
                {
                    flags.benchmarkMode = true;
                    flags.benchmarkOptions.singleScript = arg.substr(std::string("--benchmark=").size());
                }
                else if (arg.rfind("--benchmark-lexer=", 0) == 0)
                {
                    flags.benchmarkMode = true;
                    flags.benchmarkOptions.singleLexerScript = arg.substr(std::string("--benchmark-lexer=").size());
                }
                else if (arg.rfind("--benchmark-iterations=", 0) == 0)
                {
                    flags.benchmarkMode = true;
                    try
                    {
                        int n = std::stoi(arg.substr(std::string("--benchmark-iterations=").size()));
                        if (n > 0) flags.benchmarkOptions.measuredIterations = n;
                    }
                    catch (...)
                    {
                        std::cerr << "Warning: invalid --benchmark-iterations value, using default\n";
                    }
                }
                else if (arg.rfind("--benchmark-output=", 0) == 0)
                {
                    flags.benchmarkMode = true;
                    std::string fmt = arg.substr(std::string("--benchmark-output=").size());
                    if (fmt == "json")
                    {
                        flags.benchmarkOptions.outputFormat = runMain::BenchmarkOutput::Json;
                    }
                    else if (fmt == "text")
                    {
                        flags.benchmarkOptions.outputFormat = runMain::BenchmarkOutput::Text;
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
                    flags.filename = arg;
                }
            }

            return flags;
        }
    }

    int executeDefaultScript(int argc, char* argv[], constants::ExecutionMode execMode)
    {
        ExecutionFlags flags = parseExecutionFlags(argc, argv);

        if (flags.benchmarkMode)
        {
            flags.benchmarkOptions.jitEnabled = flags.enableJit;
            flags.benchmarkOptions.printJitStats = flags.printJitStats;
            // MYT-251 (Phase C1): runBenchmarks runs OUTSIDE the existing
            // script-execution try/catch below, so any C++ exception escaping
            // the benchmark loop went straight to std::set_terminate (or
            // bypassed everything via the asmjit unwind hole). Wrap and
            // rethrow so the typed exception name surfaces at the actual
            // escape point — the existing handlers still produce their
            // secondary diagnostic if rethrow propagates.
            try
            {
                int rc = runMain::runBenchmarks(flags.benchmarkOptions);
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

        if (flags.filename.empty())
        {
            std::cerr << "Error: No script file specified\n";
            std::cerr << "Use --help for usage information\n";
            return 1;
        }

        // Run in debug mode if --debug flag present
        if (flags.debugMode)
        {
            runInDebugMode(flags.filename, execMode);
            return 0;
        }

        GcRuntimeGuard guard;
        try
        {
            // Initialize profiler if requested
            if (flags.profileMode != vm::profiler::ProfilerMode::DISABLED)
            {
                vm::profiler::ProfilerContext::initialize(flags.profileMode, flags.profileOutputFormat);
                guard.enableProfiler();
            }

            services::ScriptInterpreter interpreter(execMode);

            if (flags.enableJit)
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

            // MYT-310 — walk upward from the script's directory looking for an
            // ambient .mtproj so `mType.exe script.mt` resolves `@pkg/...`
            // imports identically to `mType.exe --build`. Falls through silently
            // when no project is found (one-off scripts keep working as before).
            interpreter.tryLoadAmbientProject(flags.filename);

            interpreter.runScript(flags.filename);

            // MYT-35 follow-up — drain any non-fatal compile-time warnings
            // (unused variables, missing @Override, etc.) and render them
            // through the same Rust-style renderer used for errors. Done
            // after runScript so the user sees output first, then warnings.
            runMain::reportWarnings(interpreter.getCompilerWarnings());

            // Force a GC collection at program end to detect any remaining cycles
            gc::GC::forceCollect();

            // Print GC statistics if requested
            if (flags.printGCStats)
            {
                gc::GC::printStats();
            }

            // Print JIT statistics if requested
            if (flags.printJitStats)
            {
                interpreter.getVM()->printJitStats();
            }

            // Generate profiler report if profiling was enabled
            if (flags.profileMode != vm::profiler::ProfilerMode::DISABLED)
            {
                auto& profilerCtx = vm::profiler::ProfilerContext::getInstance();
                profilerCtx.finalize();
                vm::profiler::ProfilerReport::generate(profilerCtx);
            }

            // Normal path: explicit shutdown propagates 3/4 exit codes
            // (preserves the original post-main try/catch exit-code semantics).
            int cleanupRc = guard.releaseAndShutdown();
            return cleanupRc;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            guard.releaseAndShutdown();
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
            guard.releaseAndShutdown();
            return 2;
        }
    }
}
