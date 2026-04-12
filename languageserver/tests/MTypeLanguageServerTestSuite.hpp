#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
class MTypeLanguageServerTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
