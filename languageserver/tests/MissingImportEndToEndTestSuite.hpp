#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
// MYT-445 — the MYT-47 "Add import" quick fix driven by real diagnostics
// from DocumentManager (MYT-440), not synthetic ones.
class MissingImportEndToEndTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
