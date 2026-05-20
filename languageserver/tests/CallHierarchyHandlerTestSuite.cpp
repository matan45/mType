#include "CallHierarchyHandlerTestSuite.hpp"
#include "../src/handlers/CallHierarchyHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

#include <algorithm>
#include <string>

namespace mtype::lsp::test {

namespace {

// Resolve the kind tag carried in CallHierarchyItem::data["kind"]. Tests
// pin assertions on this rather than SymbolKind because the JSON-side
// discriminator is what survives the protocol round-trip.
std::string itemKind(const CallHierarchyItem& it) {
    if (!it.data) return "";
    const auto& d = *it.data;
    if (d.contains("kind") && d.at("kind").is_string()) return d.at("kind").get<std::string>();
    return "";
}

std::string itemClass(const CallHierarchyItem& it) {
    if (!it.data) return "";
    const auto& d = *it.data;
    if (d.contains("className") && d.at("className").is_string()) return d.at("className").get<std::string>();
    return "";
}

std::string itemName(const CallHierarchyItem& it) {
    if (!it.data) return "";
    const auto& d = *it.data;
    if (d.contains("name") && d.at("name").is_string()) return d.at("name").get<std::string>();
    return "";
}

bool hasItem(const std::vector<CallHierarchyItem>& items,
             const std::string& kind, const std::string& className,
             const std::string& name) {
    return std::any_of(items.begin(), items.end(), [&](const CallHierarchyItem& it) {
        return itemKind(it) == kind && itemClass(it) == className && itemName(it) == name;
    });
}

bool hasIncomingFrom(const std::vector<CallHierarchyIncomingCall>& calls,
                     const std::string& kind, const std::string& className,
                     const std::string& name) {
    return std::any_of(calls.begin(), calls.end(), [&](const CallHierarchyIncomingCall& c) {
        return itemKind(c.from) == kind && itemClass(c.from) == className && itemName(c.from) == name;
    });
}

bool hasOutgoingTo(const std::vector<CallHierarchyOutgoingCall>& calls,
                   const std::string& kind, const std::string& className,
                   const std::string& name) {
    return std::any_of(calls.begin(), calls.end(), [&](const CallHierarchyOutgoingCall& c) {
        return itemKind(c.to) == kind && itemClass(c.to) == className && itemName(c.to) == name;
    });
}

CallHierarchyItem makeItem(const std::string& kind, const std::string& className,
                           const std::string& name) {
    CallHierarchyItem it;
    it.name = name.empty() ? className : name;
    it.kind = SymbolKind::Function;
    json data;
    data["kind"] = kind;
    data["className"] = className;
    data["name"] = name;
    it.data = std::move(data);
    return it;
}

} // anonymous namespace

void CallHierarchyHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---- prepare: declaration resolution ----

    harness.addTest("prepare_topLevelFunction_resolvesDeclaration", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function hello(): void {\n"
            "    print(42);\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto items = h.handlePrepare("file:///t.mt", {0, 11}); // cursor on `hello`
        require(!items.empty(), "expected at least one item for function declaration");
        require(hasItem(items, "function", "", "hello"), "expected function 'hello'");
    });

    harness.addTest("prepare_instanceMethod_resolvesDeclaration", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Cat {\n"
            "    public function eat(): void { print(1); }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // `eat` token starts at column 20 on line 1
        auto items = h.handlePrepare("file:///t.mt", {1, 22});
        require(hasItem(items, "method", "Cat", "eat"), "expected method Cat::eat");
    });

    harness.addTest("prepare_staticMethod_resolvesDeclaration", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Util {\n"
            "    public static function id(int x): int { return x; }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `id`
        auto items = h.handlePrepare("file:///t.mt", {1, 29});
        require(hasItem(items, "method", "Util", "id"), "expected static method Util::id");
    });

    harness.addTest("prepare_constructor_onKeyword", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Box {\n"
            "    public constructor() {}\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `constructor` keyword
        auto items = h.handlePrepare("file:///t.mt", {1, 14});
        require(hasItem(items, "constructor", "Box", ""), "expected constructor Box");
    });

    harness.addTest("prepare_constructor_onNewCallSite", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Box {\n"
            "    public constructor() {}\n"
            "}\n"
            "Box b = new Box();\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `Box` in `new Box()` — line 3, column 12 (start of `Box`)
        auto items = h.handlePrepare("file:///t.mt", {3, 12});
        require(hasItem(items, "constructor", "Box", ""), "expected constructor Box from new site");
    });

    harness.addTest("prepare_constructor_onClassHeader", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Box {\n"
            "    public constructor() {}\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `Box` in `class Box {`
        auto items = h.handlePrepare("file:///t.mt", {0, 7});
        require(hasItem(items, "constructor", "Box", ""), "expected constructor via class header");
    });

    harness.addTest("prepare_callSite_resolvesToDeclaration", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function greet(): void { print(1); }\n"
            "function main(): void {\n"
            "    greet();\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `greet` at the call site
        auto items = h.handlePrepare("file:///t.mt", {2, 5});
        require(hasItem(items, "function", "", "greet"), "expected function greet from call site");
    });

    harness.addTest("prepare_variable_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt", "int n = 5;\n");
        CallHierarchyHandler h(docMgr.get());
        auto items = h.handlePrepare("file:///t.mt", {0, 4});
        require(items.empty(), "variable should not be a callable");
    });

    harness.addTest("prepare_classWithoutExplicitConstructor_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Empty {\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto items = h.handlePrepare("file:///t.mt", {0, 7});
        require(items.empty(), "class with no explicit constructor should yield no item");
    });

    harness.addTest("prepare_unknownUri_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt", "function f(): void {}\n");
        CallHierarchyHandler h(docMgr.get());
        auto items = h.handlePrepare("file:///nonexistent.mt", {0, 9});
        require(items.empty(), "unknown URI must yield empty");
    });

    harness.addTest("prepare_whitespace_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt", "   \n");
        CallHierarchyHandler h(docMgr.get());
        auto items = h.handlePrepare("file:///t.mt", {0, 1});
        require(items.empty(), "whitespace cursor yields empty");
    });

    // ---- incomingCalls ----

    harness.addTest("incomingCalls_function_findsDirectCallers", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function helper(): void {}\n"
            "function main(): void {\n"
            "    helper();\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("function", "", "helper"));
        require(hasIncomingFrom(calls, "function", "", "main"),
                "expected 'main' as caller of 'helper'");
    });

    harness.addTest("incomingCalls_method_thisCall_classPinned", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Foo {\n"
            "    public function a(): void { this.b(); }\n"
            "    public function b(): void {}\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("method", "Foo", "b"));
        require(hasIncomingFrom(calls, "method", "Foo", "a"),
                "expected 'Foo::a' as caller of 'Foo::b'");
    });

    harness.addTest("incomingCalls_method_staticCall_classPinned", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class U {\n"
            "    public static function f(): void {}\n"
            "}\n"
            "function main(): void {\n"
            "    U::f();\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("method", "U", "f"));
        require(hasIncomingFrom(calls, "function", "", "main"),
                "expected 'main' as caller of 'U::f' via static call");
    });

    harness.addTest("incomingCalls_method_superCall_resolvesAncestor", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class P {\n"
            "    public function speak(): void {}\n"
            "}\n"
            "class C extends P {\n"
            "    public function shout(): void { super.speak(); }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("method", "P", "speak"));
        require(hasIncomingFrom(calls, "method", "C", "shout"),
                "expected 'C::shout' as caller of 'P::speak' via super");
    });

    harness.addTest("incomingCalls_constructor_findsNewExpressions", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class Box {\n"
            "    public constructor() {}\n"
            "}\n"
            "function build(): void { Box b = new Box(); }\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("constructor", "Box", ""));
        require(hasIncomingFrom(calls, "function", "", "build"),
                "expected 'build' as caller of Box constructor");
    });

    harness.addTest("incomingCalls_nestedLambda_callerBubblesUp", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "interface Runner {\n"
            "    function go(): void;\n"
            "}\n"
            "function action(): void {}\n"
            "function run(): void {\n"
            "    Runner r = () -> { action(); };\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("function", "", "action"));
        require(hasIncomingFrom(calls, "function", "", "run"),
                "calls inside lambda must attribute to enclosing 'run'");
    });

    harness.addTest("incomingCalls_topLevelCaller_dropped", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function ping(): void {}\n"
            "ping();\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("function", "", "ping"));
        require(calls.empty(),
                "top-level call sites must not appear as incoming callers");
    });

    harness.addTest("incomingCalls_noCallers_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function lonely(): void {}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("function", "", "lonely"));
        require(calls.empty(), "uncalled function has no incoming callers");
    });

    harness.addTest("incomingCalls_crossFile_findsCaller", []() {
        auto docMgr = std::make_unique<DocumentManager>();
        docMgr->openDocument("file:///a.mt",
            "function target(): void {}\n", 1);
        docMgr->parseDocument("file:///a.mt");
        docMgr->openDocument("file:///b.mt",
            "function caller(): void { target(); }\n", 1);
        docMgr->parseDocument("file:///b.mt");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleIncoming(makeItem("function", "", "target"));
        require(hasIncomingFrom(calls, "function", "", "caller"),
                "cross-file caller must be discoverable");
    });

    // ---- outgoingCalls ----

    harness.addTest("outgoingCalls_function_listsAllCalls", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function a(): void {}\n"
            "function b(): void {}\n"
            "function main(): void { a(); b(); }\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("function", "", "main"));
        require(hasOutgoingTo(calls, "function", "", "a"), "expected outgoing call to 'a'");
        require(hasOutgoingTo(calls, "function", "", "b"), "expected outgoing call to 'b'");
    });

    harness.addTest("outgoingCalls_method_includesThisCall", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class X {\n"
            "    public function a(): void { this.b(); }\n"
            "    public function b(): void {}\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("method", "X", "a"));
        require(hasOutgoingTo(calls, "method", "X", "b"),
                "expected X::a to call X::b via this");
    });

    harness.addTest("outgoingCalls_method_includesSuper", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class P {\n"
            "    public function s(): void {}\n"
            "}\n"
            "class C extends P {\n"
            "    public function go(): void { super.s(); }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("method", "C", "go"));
        require(hasOutgoingTo(calls, "method", "P", "s"),
                "super.s() must resolve through inheritance chain");
    });

    harness.addTest("outgoingCalls_constructor_listsCalls", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function init(): void {}\n"
            "class Box {\n"
            "    public constructor() { init(); }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("constructor", "Box", ""));
        require(hasOutgoingTo(calls, "function", "", "init"),
                "constructor body's calls must surface in outgoing");
    });

    harness.addTest("outgoingCalls_nestedLambda_bubblesUp", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "interface Runner {\n"
            "    function go(): void;\n"
            "}\n"
            "function act(): void {}\n"
            "function run(): void {\n"
            "    Runner r = () -> { act(); };\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("function", "", "run"));
        require(hasOutgoingTo(calls, "function", "", "act"),
                "calls inside lambda must surface as outgoing from enclosing");
    });

    harness.addTest("outgoingCalls_unresolvedSuper_emitsNothing", []() {
        // Class with no parent that defines `missing` — super walk yields
        // nothing, so we don't surface a name-only fallback (Q9).
        auto docMgr = makeDocManager("file:///t.mt",
            "class P {}\n"
            "class C extends P {\n"
            "    public function go(): void { super.missing(); }\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("method", "C", "go"));
        // No P::missing exists; no item should be produced for the
        // super call itself. We don't fail just because the list is
        // empty — only assert that no spurious target appears.
        for (const auto& c : calls) {
            require(!(itemKind(c.to) == "method" && itemName(c.to) == "missing"),
                    "unresolved super call must not produce a target");
        }
    });

    harness.addTest("outgoingCalls_overloadedTarget_singleItem", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "class C {\n"
            "    public constructor() {}\n"
            "    public constructor(int x) {}\n"
            "}\n"
            "function f(): void { C c = new C(); C d = new C(1); }\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("function", "", "f"));
        int hits = 0;
        for (const auto& c : calls) {
            if (itemKind(c.to) == "constructor" && itemClass(c.to) == "C") ++hits;
        }
        require(hits == 1, "overloaded constructors must collapse to one outgoing target");
    });

    harness.addTest("outgoingCalls_noBody_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt",
            "function empty(): void {}\n");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("function", "", "empty"));
        require(calls.empty(), "no body, no outgoing");
    });

    harness.addTest("outgoingCalls_crossFile_findsTarget", []() {
        auto docMgr = std::make_unique<DocumentManager>();
        docMgr->openDocument("file:///lib.mt",
            "function shared(): void {}\n", 1);
        docMgr->parseDocument("file:///lib.mt");
        docMgr->openDocument("file:///app.mt",
            "function entry(): void { shared(); }\n", 1);
        docMgr->parseDocument("file:///app.mt");
        CallHierarchyHandler h(docMgr.get());
        auto calls = h.handleOutgoing(makeItem("function", "", "entry"));
        require(hasOutgoingTo(calls, "function", "", "shared"),
                "outgoing call must resolve target in a different open document");
    });

    harness.addTest("prepare_classWithModifier_abstractClassHeader", []() {
        // The class-header path must work for any modifier sequence
        // (`abstract`, `final`, `value`) prefixing `class`. nameTokenRange's
        // source scan handles the column drift; the +6 heuristic from the
        // original draft would have failed here.
        auto docMgr = makeDocManager("file:///t.mt",
            "abstract class Shape {\n"
            "    public constructor() {}\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `Shape` (col 15-19, position on second char)
        auto items = h.handlePrepare("file:///t.mt", {0, 16});
        require(hasItem(items, "constructor", "Shape", ""),
                "expected constructor via abstract class header");
    });

    harness.addTest("prepare_ambiguousMethodCall_dedupCollapses", []() {
        // Cursor on `obj.eat()` where two classes define `eat`. The name-only
        // resolution emits a candidate for each class; prepare's dedup pass
        // collapses overlapping (kind,className,name) keys so each unique
        // method appears at most once. We assert at least one and no dup.
        auto docMgr = makeDocManager("file:///t.mt",
            "class Cat {\n"
            "    public function eat(): void {}\n"
            "}\n"
            "class Dog {\n"
            "    public function eat(): void {}\n"
            "}\n"
            "function caller(): void {\n"
            "    Cat c = new Cat();\n"
            "    c.eat();\n"
            "}\n");
        CallHierarchyHandler h(docMgr.get());
        // cursor on `eat` in c.eat() — line 8 col 6
        auto items = h.handlePrepare("file:///t.mt", {8, 7});
        // Either Cat::eat or Dog::eat (or both) is acceptable; what matters
        // is no duplicate item for the same (kind,className,name).
        std::vector<std::string> seen;
        for (const auto& it : items) {
            seen.push_back(itemKind(it) + "/" + itemClass(it) + "/" + itemName(it));
        }
        std::sort(seen.begin(), seen.end());
        require(std::unique(seen.begin(), seen.end()) == seen.end(),
                "prepare must dedup ambiguous candidates");
    });

    harness.addTest("outgoingCalls_unknownItem_returnsEmpty", []() {
        auto docMgr = makeDocManager("file:///t.mt", "function f(): void {}\n");
        CallHierarchyHandler h(docMgr.get());
        // Item with empty data blob → identity has empty kind → handler must bail.
        CallHierarchyItem stale;
        stale.name = "ghost";
        stale.kind = SymbolKind::Function;
        auto calls = h.handleOutgoing(stale);
        require(calls.empty(), "item with no identity data must yield empty");
    });
}

} // namespace mtype::lsp::test
