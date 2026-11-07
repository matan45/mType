#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
// MYT-440 — unresolved-type diagnostics through the real DocumentManager.
class UnresolvedTypeAnalyzerTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
