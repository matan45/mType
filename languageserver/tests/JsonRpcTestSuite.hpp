#pragma once
#include "LspTestHarness.hpp"

namespace mtype::lsp::test {
class JsonRpcTestSuite {
public:
    void registerTests(LspTestHarness& harness);
};
} // namespace mtype::lsp::test
