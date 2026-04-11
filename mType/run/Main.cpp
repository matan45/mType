#include "TestUtilities.hpp"
#include "DebugSession.hpp"
#include "ScriptAnalyzer.hpp"
#include "ErrorReporting.hpp"

#include "../diagnostics/DiagnosticRenderer.hpp"

#include "../gc/GC.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"
#include "../project/ProjectConfigParser.hpp"
#include "../project/ProjectBuilder.hpp"
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

using namespace parser;
using namespace lexer;
using namespace services;
using namespace environment;

int main(int argc, char* argv[])
{
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

    // Check for test suite execution
    if (argc >= 2 && std::string(argv[argc - 2]) == "--test" && argc >= 3)
    {
        std::string suiteName = argv[argc - 1];
        runSpecificTestSuite(suiteName, execMode);
        return 0;
    }

    if (argc >= 2 && std::string(argv[argc - 1]) == "--tests")
    {
        runAllTests(execMode);
        return 0;
    }

    if (argc == 2 && std::string(argv[1]) == "--tests")
    {
        runAllTests(execMode);
        return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--test")
    {
        std::string suiteName = argv[2];
        runSpecificTestSuite(suiteName, execMode);

        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--help")
    {
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
        std::cout << "  " << argv[0] << " --clean [project.mtproj]   - Remove compiled bytecode files\n";
        std::cout << "  " << argv[0] <<
            " --init <name> <include>    - Create new .mtproj file (e.g. --init MyApp src/**/*.mt)\n";
        std::cout << "  " << argv[0] << " --add <pattern> [.mtproj]  - Add include pattern to project\n";
        std::cout << "  " << argv[0] << " --remove <pattern> [.mtproj] - Remove include pattern from project\n";
        std::cout << "  " << argv[0] <<
            " --find-script-classes <script.mt> - Analyze script and show all @Script classes\n";
        std::cout << "  " << argv[0] <<
            " --test-script-objects <script.mt> - Demo: Create objects and call methods from C++\n";
        std::cout << "  " << argv[0] << " --tests                    - Run all test suites\n";
        std::cout << "  " << argv[0] << " --test <suite>             - Run specific test suite\n";
        std::cout << "  " << argv[0] << " --help                     - Show this help message\n\n";
        printAvailableTestSuites();
        return 0;
    }

    // Handle --build command (project compilation)
    if (argc >= 2 && std::string(argv[1]) == "--build")
    {
        std::string mtprojPath;
        bool buildLib = false;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--lib")
            {
                buildLib = true;
            }
            else if (arg[0] != '-')
            {
                mtprojPath = arg;
            }
        }

        try
        {
            project::ProjectConfigParser parser;

            if (mtprojPath.empty())
            {
                auto found = parser.findProject(".");
                if (!found)
                {
                    std::cerr << "Error: No .mtproj file found in current directory or parents\n";
                    return 1;
                }
                mtprojPath = *found;
            }

            std::cout << "Loading project: " << mtprojPath << "\n";

            auto config = parser.parse(mtprojPath);

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
                // Build into single library file
                std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();
                std::cout << "Building library: " << libPath << "\n\n";
                result = builder.buildLibrary(*config, libPath);
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
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    // Handle --clean command
    if (argc >= 2 && std::string(argv[1]) == "--clean")
    {
        std::string mtprojPath;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg[0] != '-')
            {
                mtprojPath = arg;
                break;
            }
        }

        try
        {
            project::ProjectConfigParser parser;

            if (mtprojPath.empty())
            {
                auto found = parser.findProject(".");
                if (!found)
                {
                    std::cerr << "Error: No .mtproj file found in current directory or parents\n";
                    return 1;
                }
                mtprojPath = *found;
            }

            auto config = parser.parse(mtprojPath);

            project::ProjectBuilder builder;
            builder.clean(*config);

            std::cout << "Clean completed. Removed: " << config->output.directory << "\n";
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

        interpreter.runScript(filename);

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

    // Clean up GC to avoid static destruction order issues
    gc::GC::shutdown();

    // Cleanup reflection static state to avoid static destruction order issues
    reflection::ReflectionNatives::cleanup();
    json::JsonNatives::cleanup();

    return 0;
}
