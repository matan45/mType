#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
// MYT-440 — false-positive sweep: the unresolved-type analyzer must report
// nothing on the stdlib (mType/tests/testFiles/lib) and examples/.
class UnresolvedTypeSweepTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
