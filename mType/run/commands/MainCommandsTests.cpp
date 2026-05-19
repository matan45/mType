#include "MainCommands.hpp"

#include "../TestUtilities.hpp"

#include <string>

namespace runMain::commands
{
    std::optional<int> handleTestCommand(int argc, char* argv[],
                                         constants::ExecutionMode execMode,
                                         bool testJitEnabled)
    {
        auto isTestFlag = [](const std::string& s) {
            return s == "--no-jit" || s == "--tests" || s == "--test";
        };

        // `--test <suite>` anywhere in argv (so `--test integration --no-jit` and
        // `--no-jit --test integration` both work). Checked first so that an
        // explicit suite name isn't masked by the bare `--tests` scan below.
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
            if (std::string(argv[i]) == "--tests")
            {
                int failures = runAllTests(execMode, testJitEnabled);
                return failures == 0 ? 0 : 1;
            }
        }

        return std::nullopt;
    }
}
