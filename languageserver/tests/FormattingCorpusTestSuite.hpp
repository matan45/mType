#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
// MYT-443: formats real mType sources and checks that formatting is idempotent
// and never changes the token stream the core lexer produces.
class FormattingCorpusTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
