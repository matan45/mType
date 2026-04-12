#include "MTypeLanguageServerTestSuite.hpp"
#include "../src/MTypeLanguageServer.hpp"

namespace mtype::lsp::test {

void MTypeLanguageServerTestSuite::registerTests(LspTestHarness& harness) {

    // All dispatch methods in MTypeLanguageServer are private.
    // The only public method is run() which blocks on stdin.
    // Individual handlers are tested directly in their own suites.
    // These smoke tests verify construction and component wiring.

    harness.addTest("constructor does not crash", []() {
        MTypeLanguageServer server;
        // If we get here, construction succeeded — all handlers wired up
    });

    harness.addTest("server object is destructible", []() {
        {
            MTypeLanguageServer server;
        }
        // Destructor ran without crash
    });
}

} // namespace mtype::lsp::test
