#include "DiagnosticsHandlerTestSuite.hpp"
#include "../src/handlers/DiagnosticsHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

#include <string>
#include <vector>

namespace mtype::lsp::test {

void DiagnosticsHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // publishDiagnostics flow:
    //   1. Get document (no-op if null)
    //   2. Call analyzeDiagnostics(doc) which converts core diags + validates import paths
    //   3. Call publisher_(uri, diags) if publisher_ is set

    harness.addTest("publishDiagnostics calls publisher with converted diagnostics", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class { broken");
        DiagnosticsHandler handler(docMgr.get());

        bool callbackCalled = false;
        std::string capturedUri;
        std::vector<Diagnostic> capturedDiags;

        handler.setPublisher([&](const std::string& uri, const std::vector<Diagnostic>& diags) {
            callbackCalled = true;
            capturedUri = uri;
            capturedDiags = diags;
        });

        handler.publishDiagnostics("file:///test.mt");

        require(callbackCalled, "publisher callback should have been called");
        require(capturedUri == "file:///test.mt", "URI mismatch in callback");
        // Parse error in "class { broken" should produce at least one diagnostic
        require(!capturedDiags.empty(), "expected diagnostics for parse error");
    });

    harness.addTest("no publisher set: publishDiagnostics runs analyzeDiagnostics but does not crash", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        DiagnosticsHandler handler(docMgr.get());
        // No setPublisher call — analyzeDiagnostics runs but nothing published
        handler.publishDiagnostics("file:///test.mt");
        // If we get here, no crash
    });

    harness.addTest("null document: publishDiagnostics is a no-op", []() {
        DocumentManager docMgr;
        // Don't open any document
        DiagnosticsHandler handler(&docMgr);

        bool called = false;
        handler.setPublisher([&](const std::string&, const std::vector<Diagnostic>&) {
            called = true;
        });

        handler.publishDiagnostics("file:///nonexistent.mt");
        // Doc is null → early return before publisher is called
        require(!called, "publisher should not be called for null document");
    });

    harness.addTest("valid document publishes diagnostics with no errors", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        DiagnosticsHandler handler(docMgr.get());

        std::vector<Diagnostic> capturedDiags;
        handler.setPublisher([&](const std::string&, const std::vector<Diagnostic>& diags) {
            capturedDiags = diags;
        });

        handler.publishDiagnostics("file:///test.mt");
        // Valid 'class Foo {}' should produce no error-severity diagnostics
        for (const auto& d : capturedDiags) {
            require(d.severity != 1,
                "valid 'class Foo {}' should not produce errors, got: " + d.message);
        }
    });

    harness.addTest("diagnostics have source set to mType", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class { broken");
        DiagnosticsHandler handler(docMgr.get());

        std::vector<Diagnostic> capturedDiags;
        handler.setPublisher([&](const std::string&, const std::vector<Diagnostic>& diags) {
            capturedDiags = diags;
        });

        handler.publishDiagnostics("file:///test.mt");

        for (const auto& d : capturedDiags) {
            if (d.source.has_value()) {
                require(*d.source == "mType",
                    "diagnostic source should be 'mType', got '" + *d.source + "'");
            }
        }
    });

    harness.addTest("warning diagnostics are published with warning severity", []() {
        auto docMgr = makeDocManager("file:///test.mt", "int unused = 1;\n");
        DiagnosticsHandler handler(docMgr.get());

        std::vector<Diagnostic> capturedDiags;
        handler.setPublisher([&](const std::string&, const std::vector<Diagnostic>& diags) {
            capturedDiags = diags;
        });

        handler.publishDiagnostics("file:///test.mt");

        bool sawWarning = false;
        for (const auto& d : capturedDiags) {
            if (d.severity == 2) {
                sawWarning = true;
                require(d.source.has_value() && *d.source == "mType",
                    "warning diagnostic source should be 'mType'");
                if (d.code.has_value()) {
                    require(d.code->get<std::string>() == "MT-W2001",
                        "expected unused-variable warning code MT-W2001");
                }
            }
        }

        require(sawWarning, "expected at least one warning-severity diagnostic");
    });
}

} // namespace mtype::lsp::test
