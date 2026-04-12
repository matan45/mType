#include "LspTestHarness.hpp"
#include "CompletionHandlerTestSuite.hpp"
#include "CodeActionHandlerTestSuite.hpp"

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string suiteFilter;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
            suiteFilter = argv[++i];
        }
    }

    int totalFailures = 0;

    auto shouldRun = [&](const std::string& name) {
        return suiteFilter.empty() || name == suiteFilter;
    };

    if (shouldRun("completion")) {
        mtype::lsp::test::LspTestHarness harness("CompletionHandler Tests");
        mtype::lsp::test::CompletionHandlerTestSuite suite;
        suite.registerTests(harness);
        totalFailures += harness.runAll();
    }

    if (shouldRun("codeaction")) {
        mtype::lsp::test::LspTestHarness harness("CodeActionHandler Tests");
        mtype::lsp::test::CodeActionHandlerTestSuite suite;
        suite.registerTests(harness);
        totalFailures += harness.runAll();
    }

    std::cout << "\n========================================\n";
    if (totalFailures == 0) {
        std::cout << "All tests passed.\n";
    } else {
        std::cout << totalFailures << " test(s) failed.\n";
    }

    return totalFailures > 0 ? 1 : 0;
}
