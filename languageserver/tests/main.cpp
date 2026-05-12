#include "LspTestHarness.hpp"

// Phase 1: Pure function suites
#include "LSPTypesTestSuite.hpp"
#include "JsonRpcTestSuite.hpp"
#include "UriUtilsTestSuite.hpp"
#include "LspDiagnosticConverterTestSuite.hpp"
#include "FormattingHandlerTestSuite.hpp"

// Phase 2: Core component suites
#include "DocumentManagerTestSuite.hpp"
#include "WorkspaceSymbolIndexTestSuite.hpp"

// Phase 3: Handler suites
#include "CompletionHandlerTestSuite.hpp"
#include "CodeActionHandlerTestSuite.hpp"
#include "HoverHandlerTestSuite.hpp"
#include "DefinitionHandlerTestSuite.hpp"
#include "DiagnosticsHandlerTestSuite.hpp"
#include "CodeLensHandlerTestSuite.hpp"
#include "ReferencesHandlerTestSuite.hpp"
#include "RenameHandlerTestSuite.hpp"
#include "SignatureHelpHandlerTestSuite.hpp"
#include "SemanticTokensHandlerTestSuite.hpp"
#include "InlayHintHandlerTestSuite.hpp"
#include "DocumentSymbolHandlerTestSuite.hpp"
#include "WorkspaceSymbolHandlerTestSuite.hpp"

// Phase 4: Analysis suites
#include "SymbolRegistrationVisitorTestSuite.hpp"

// Phase 5: Filesystem-dependent suites
#include "PathCompletionHandlerTestSuite.hpp"
#include "MtFileWalkerTestSuite.hpp"
#include "ProjectConfigProviderTestSuite.hpp"
#include "ImportResolverTestSuite.hpp"

// Phase 6: Server smoke test
#include "MTypeLanguageServerTestSuite.hpp"

#include <cstring>
#include <iostream>
#include <string>

#define RUN_SUITE(filterName, displayName, SuiteClass) \
    if (shouldRun(filterName)) { \
        mtype::lsp::test::LspTestHarness harness(displayName); \
        mtype::lsp::test::SuiteClass suite; \
        suite.registerTests(harness); \
        totalFailures += harness.runAll(); \
    }

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

    // Phase 1: Pure function suites
    RUN_SUITE("lsptypes", "LSPTypes Tests", LSPTypesTestSuite)
    RUN_SUITE("jsonrpc", "JsonRpc Tests", JsonRpcTestSuite)
    RUN_SUITE("uriutils", "UriUtils Tests", UriUtilsTestSuite)
    RUN_SUITE("diagnosticconverter", "LspDiagnosticConverter Tests", LspDiagnosticConverterTestSuite)
    RUN_SUITE("formatting", "FormattingHandler Tests", FormattingHandlerTestSuite)

    // Phase 2: Core components
    RUN_SUITE("documentmanager", "DocumentManager Tests", DocumentManagerTestSuite)
    RUN_SUITE("workspacesymbolindex", "WorkspaceSymbolIndex Tests", WorkspaceSymbolIndexTestSuite)

    // Phase 3: Handlers
    RUN_SUITE("completion", "CompletionHandler Tests", CompletionHandlerTestSuite)
    RUN_SUITE("codeaction", "CodeActionHandler Tests", CodeActionHandlerTestSuite)
    RUN_SUITE("hover", "HoverHandler Tests", HoverHandlerTestSuite)
    RUN_SUITE("definition", "DefinitionHandler Tests", DefinitionHandlerTestSuite)
    RUN_SUITE("diagnostics", "DiagnosticsHandler Tests", DiagnosticsHandlerTestSuite)
    RUN_SUITE("codelens", "CodeLensHandler Tests", CodeLensHandlerTestSuite)
    RUN_SUITE("references", "ReferencesHandler Tests", ReferencesHandlerTestSuite)
    RUN_SUITE("rename", "RenameHandler Tests", RenameHandlerTestSuite)
    RUN_SUITE("signaturehelp", "SignatureHelpHandler Tests", SignatureHelpHandlerTestSuite)
    RUN_SUITE("semantictokens", "SemanticTokensHandler Tests", SemanticTokensHandlerTestSuite)
    RUN_SUITE("inlayhint", "InlayHintHandler Tests", InlayHintHandlerTestSuite)
    RUN_SUITE("documentsymbol", "DocumentSymbolHandler Tests", DocumentSymbolHandlerTestSuite)
    RUN_SUITE("workspacesymbol", "WorkspaceSymbolHandler Tests", WorkspaceSymbolHandlerTestSuite)

    // Phase 4: Analysis
    RUN_SUITE("symbolregistration", "SymbolRegistrationVisitor Tests", SymbolRegistrationVisitorTestSuite)

    // Phase 5: Filesystem-dependent
    RUN_SUITE("pathcompletion", "PathCompletionHandler Tests", PathCompletionHandlerTestSuite)
    RUN_SUITE("mtfilewalker", "MtFileWalker Tests", MtFileWalkerTestSuite)
    RUN_SUITE("projectconfig", "ProjectConfigProvider Tests", ProjectConfigProviderTestSuite)
    RUN_SUITE("importresolver", "ImportResolver Tests", ImportResolverTestSuite)

    // Phase 6: Server smoke test
    RUN_SUITE("server", "MTypeLanguageServer Tests", MTypeLanguageServerTestSuite)

    std::cout << "\n========================================\n";
    if (totalFailures == 0) {
        std::cout << "All tests passed.\n";
    } else {
        std::cout << totalFailures << " test(s) failed.\n";
    }

    return totalFailures > 0 ? 1 : 0;
}
