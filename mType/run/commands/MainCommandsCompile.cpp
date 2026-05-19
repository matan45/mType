#include "MainCommands.hpp"

#include "../ErrorReporting.hpp"
#include "../ScriptAnalyzer.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleCompileCommand(int argc, char* argv[])
    {
        // --compile may appear anywhere in argv (preserves original scan).
        int compileIdx = -1;
        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--compile")
            {
                compileIdx = i;
                break;
            }
        }
        if (compileIdx < 0) return std::nullopt;

        std::string sourceFile;
        for (int j = compileIdx + 1; j < argc; ++j)
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

            services::ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);
            interpreter.compileToFile(sourceFile, outputFile);
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    std::optional<int> handleRunCachedCommand(int argc, char* argv[])
    {
        if (argc != 3 || std::string(argv[1]) != "--run-cached")
        {
            return std::nullopt;
        }

        std::string cachedFile = argv[2];

        try
        {
            std::cout << "Loading cached bytecode from " << cachedFile << "...\n";

            services::ScriptInterpreter interpreter;
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

    std::optional<int> handleTestScriptObjectsCommand(int argc, char* argv[])
    {
        if (argc < 3 || std::string(argv[1]) != "--test-script-objects")
        {
            return std::nullopt;
        }

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

    std::optional<int> handleFindScriptClassesCommand(int argc, char* argv[])
    {
        if (argc < 3 || std::string(argv[1]) != "--find-script-classes")
        {
            return std::nullopt;
        }

        std::string scriptFile = argv[2];

        try
        {
            std::cout << "Analyzing script: " << scriptFile << "\n";
            std::cout << "(Parsing and registering classes for analysis)\n\n";

            services::ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);

            // Parse and register classes without executing the script
            interpreter.parseAndRegisterClasses(scriptFile);

            // Query and print all @Script annotated classes
            auto env = interpreter.getEnvironment();
            printScriptAnnotatedClasses(env);

            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }
}
