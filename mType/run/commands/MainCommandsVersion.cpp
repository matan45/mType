#include "MainCommands.hpp"

#include "../TestUtilities.hpp"
#include "../../version/Version.hpp"

#include <iostream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleVersionCommand(int argc, char* argv[])
    {
        if (argc >= 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v"))
        {
            std::cout << "mType " << mType::version::getVersionString() << "\n";
            return 0;
        }
        return std::nullopt;
    }

    std::optional<int> handleHelpCommand(int argc, char* argv[])
    {
        if (argc < 2 || std::string(argv[1]) != "--help")
        {
            return std::nullopt;
        }

        std::cout << "mType " << mType::version::getVersionString() << "\n\n";
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <script_file.mt>           - Run a script file\n";
        std::cout << "  " << argv[0] << " --debug <script.mt>        - Run with debugger (breakpoints, stepping)\n";
        std::cout << "  " << argv[0] << " --gc-stats <script.mt>     - Run and print GC statistics after execution\n";
        std::cout << "  " << argv[0] << " --jit-stats <script.mt>    - Run and print JIT statistics after execution\n";
        std::cout << "  " << argv[0] << " --no-jit <script.mt>       - Disable JIT compilation (JIT is on by default)\n";
        std::cout << "  " << argv[0] << " --no-stack-scope-release   - Treat STACK_SCOPE_ENTER/LEAVE as no-ops (A/B testing)\n";
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
        std::cout << "  " << argv[0] << " --build --exe --gui [.mtproj] - As above, but windowed-subsystem launcher (no console window on Explorer launch)\n";
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
}
