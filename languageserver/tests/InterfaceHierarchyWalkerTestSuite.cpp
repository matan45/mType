#include "InterfaceHierarchyWalkerTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/InterfaceHierarchyWalker.hpp"
#include "TestFixtures.hpp"

#include <algorithm>
#include <vector>

namespace mtype::lsp::test {

namespace {

const char* kHierarchySource =
    "interface A {\n"
    "    function a(): void;\n"
    "}\n"
    "interface B extends A {\n"
    "    function b(): void;\n"
    "}\n"
    "interface C extends B {\n"
    "    function c(): void;\n"
    "}\n"
    "interface L extends A {\n"
    "    function l(): void;\n"
    "}\n"
    "interface D extends C, L {\n"
    "    function d(): void;\n"
    "}\n"
    "class Impl {\n"
    "    public function get(): Impl {\n"
    "        return this;\n"
    "    }\n"
    "}\n"
    "interface P<T> {\n"
    "    function get(): T;\n"
    "}\n"
    "interface Q extends P<Impl> {\n"
    "    function q(): void;\n"
    "}\n";

std::vector<std::string> collectVisitOrder(
    const std::string& root,
    const std::shared_ptr<environment::Environment>& env) {
    std::vector<std::string> order;
    ::types::TypeSubstitutionService subst;
    analysis::walkInterfaceHierarchy(
        root, env->getInterfaceRegistry(), subst,
        [&](const auto& iface, const auto& /*substitutions*/) {
            order.push_back(iface.getName());
        });
    return order;
}

int firstIndexOf(const std::vector<std::string>& order, const std::string& name) {
    auto it = std::find(order.begin(), order.end(), name);
    return it == order.end() ? -1 : static_cast<int>(it - order.begin());
}

} // namespace

void InterfaceHierarchyWalkerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("linear chain visits parents before the root", []() {
        auto docMgr = makeDocManager("file:///t.mt", kHierarchySource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        auto order = collectVisitOrder("C", doc->environment);
        int a = firstIndexOf(order, "A");
        int b = firstIndexOf(order, "B");
        int c = firstIndexOf(order, "C");
        require(a >= 0 && b >= 0 && c >= 0,
            "expected A, B, C all visited from root C");
        require(a < b && b < c,
            "expected parent-before-child order A < B < C");
    });

    harness.addTest("two-level chain visits exactly parent then child", []() {
        auto docMgr = makeDocManager("file:///t.mt", kHierarchySource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        auto order = collectVisitOrder("B", doc->environment);
        require(order.size() == 2, "expected exactly 2 visits from root B, got "
            + std::to_string(order.size()));
        require(order[0] == "A" && order[1] == "B",
            "expected visit order [A, B]");
    });

    harness.addTest("diamond visits every ancestor before the root", []() {
        auto docMgr = makeDocManager("file:///t.mt", kHierarchySource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        auto order = collectVisitOrder("D", doc->environment);
        // D extends C (-> B -> A) and L (-> A). The shared ancestor A may
        // legitimately be visited once per path (consumers de-duplicate with
        // an "already emitted" set), so only presence and ordering relative
        // to D are pinned here.
        int d = firstIndexOf(order, "D");
        require(d >= 0, "root D must be visited");
        require(std::count(order.begin(), order.end(), "D") == 1,
            "root D must be visited exactly once");
        for (const auto* ancestor : {"A", "B", "C", "L"}) {
            int idx = firstIndexOf(order, ancestor);
            require(idx >= 0, std::string("ancestor ") + ancestor
                + " must be visited");
            require(idx < d, std::string("ancestor ") + ancestor
                + " must be visited before root D");
        }
    });

    harness.addTest("unknown root visits nothing", []() {
        auto docMgr = makeDocManager("file:///t.mt", kHierarchySource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        auto order = collectVisitOrder("DoesNotExist", doc->environment);
        require(order.empty(), "unknown interface should produce no visits");
    });

    harness.addTest("null registry is tolerated", []() {
        ::types::TypeSubstitutionService subst;
        int visits = 0;
        analysis::walkInterfaceHierarchy(
            "A", nullptr, subst,
            [&](const auto&, const auto&) { ++visits; });
        require(visits == 0, "null registry should produce no visits");
    });

    harness.addTest("generic parent receives concrete substitutions", []() {
        auto docMgr = makeDocManager("file:///t.mt", kHierarchySource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        // Q extends P<Impl>: when the walk reaches P, its generic parameter
        // T must be bound to Impl.
        ::types::TypeSubstitutionService subst;
        bool sawP = false;
        std::string boundT;
        analysis::walkInterfaceHierarchy(
            "Q", doc->environment->getInterfaceRegistry(), subst,
            [&](const auto& iface, const auto& substitutions) {
                if (iface.getName() == "P") {
                    sawP = true;
                    auto it = substitutions.find("T");
                    if (it != substitutions.end()) {
                        boundT = it->second;
                    }
                }
            });
        require(sawP, "walk from Q must reach parent P");
        require(boundT == "Impl",
            "expected T bound to Impl, got '" + boundT + "'");
    });
}

} // namespace mtype::lsp::test
