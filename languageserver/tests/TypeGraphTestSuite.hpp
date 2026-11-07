#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
// MYT-432 — workspace type graph: edges, transitive subtypes, members of
// closed files, and the index's freshness rules as seen through the graph.
class TypeGraphTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
