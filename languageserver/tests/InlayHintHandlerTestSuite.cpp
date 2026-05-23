#include "InlayHintHandlerTestSuite.hpp"

#include <algorithm>
#include <string>

#include "../src/DocumentManager.hpp"
#include "../src/handlers/InlayHintHandler.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

namespace {

// Open-doc range that covers the whole document.
Range fullDoc() {
    Range r;
    r.start.line = 0;
    r.start.character = 0;
    r.end.line = 10000;
    r.end.character = 0;
    return r;
}

// Count hints with the given kind and label.
int countHintsWithLabel(const std::vector<InlayHint>& hints,
                        const std::string& label,
                        InlayHintKind kind) {
    int n = 0;
    for (const auto& h : hints) {
        if (h.label == label && h.kind && *h.kind == kind) n++;
    }
    return n;
}

int countHintsOfKind(const std::vector<InlayHint>& hints, InlayHintKind kind) {
    int n = 0;
    for (const auto& h : hints) {
        if (h.kind && *h.kind == kind) n++;
    }
    return n;
}

} // namespace

void InlayHintHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---- Parameter-name hints ----

    harness.addTest("parameter hints on a top-level function call", []() {
        const std::string src =
            "function draw(int width, int height): void { }\n"
            "draw(10, 20);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        int p = countHintsOfKind(hints, InlayHintKind::Parameter);
        require(p == 2,
            "expected 2 Parameter hints, got " + std::to_string(p));
        require(countHintsWithLabel(hints, "width:", InlayHintKind::Parameter) == 1,
            "expected 'width:' hint");
        require(countHintsWithLabel(hints, "height:", InlayHintKind::Parameter) == 1,
            "expected 'height:' hint");
    });

    harness.addTest("parameter hints on a method call", []() {
        const std::string src =
            "class Mover {\n"
            "    public constructor() {}\n"
            "    public function move(int dx, int dy): void { }\n"
            "}\n"
            "Mover m = new Mover();\n"
            "m.move(1, 2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "dx:", InlayHintKind::Parameter) == 1,
            "expected 'dx:' hint on method call");
        require(countHintsWithLabel(hints, "dy:", InlayHintKind::Parameter) == 1,
            "expected 'dy:' hint on method call");
    });

    harness.addTest("parameter hints on a constructor call", []() {
        const std::string src =
            "class Box {\n"
            "    public constructor(int w, int h) {}\n"
            "}\n"
            "Box b = new Box(3, 4);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "w:", InlayHintKind::Parameter) == 1,
            "expected 'w:' hint on constructor call");
        require(countHintsWithLabel(hints, "h:", InlayHintKind::Parameter) == 1,
            "expected 'h:' hint on constructor call");
    });

    harness.addTest("hint omitted when arg identifier matches param name", []() {
        // Both args at the call site are bare identifiers whose lexemes
        // match the declared parameter names — the LSP spec's
        // "no hint where it duplicates source text" rule kicks in.
        const std::string src =
            "function draw(int width, int height): void { }\n"
            "int width = 10;\n"
            "int height = 20;\n"
            "draw(width, height);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints on the call (both args duplicate names)");
    });

    harness.addTest("hint positions are correct after string-literal args (MYT-307)", []() {
        // MYT-307: a fast path in Lexer::parseStringLiteral previously
        // skipped past the literal body via `pos = end + 1` without
        // updating SourceLocationTracker, so every token AFTER a string
        // literal on the same line had a column short by (len - 1). The
        // visible symptom in VS Code was inlay hints rendering INSIDE
        // adjacent string literals. Guard that hint columns now land
        // before each argument, not somewhere mid-string.
        const std::string src =
            "function f(string a, string b, int c): void { }\n"
            "f(\"xx\", \"yy\", 1);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());

        // Call line (0-based): `f("xx", "yy", 1);`
        //  col: 0123456789...
        // expected hint positions: a -> 2, b -> 8, c -> 14
        auto findHintChar = [&](const std::string& label) -> int {
            for (const auto& h : hints) {
                if (h.label == label && h.kind
                    && *h.kind == InlayHintKind::Parameter
                    && h.position.line == 1) {
                    return h.position.character;
                }
            }
            return -1;
        };

        int aCh = findHintChar("a:");
        int bCh = findHintChar("b:");
        int cCh = findHintChar("c:");
        require(aCh == 2,
            "expected 'a:' hint at character 2, got " + std::to_string(aCh));
        require(bCh == 8,
            "expected 'b:' hint at character 8 (start of second string), got "
            + std::to_string(bCh));
        require(cCh == 14,
            "expected 'c:' hint at character 14 (start of `1`), got "
            + std::to_string(cCh));
    });

    harness.addTest("hint kept when arg is a non-bare expression", []() {
        // `width` and `height` are bare-identifier args; `outer.width`
        // is an expression composed of multiple tokens. The latter MUST
        // still get a hint.
        const std::string src =
            "class Box {\n"
            "    public constructor() {}\n"
            "    public int width;\n"
            "    public int height;\n"
            "}\n"
            "function draw(int width, int height): void { }\n"
            "Box outer = new Box();\n"
            "draw(outer.width, outer.height);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "width:", InlayHintKind::Parameter) == 1,
            "expected 'width:' hint on member-access arg");
        require(countHintsWithLabel(hints, "height:", InlayHintKind::Parameter) == 1,
            "expected 'height:' hint on member-access arg");
    });

    harness.addTest("built-in functions get no parameter hints", []() {
        // Per MYT-295 design choice, built-ins like `print` are not hinted
        // to avoid visual noise on every println-like call.
        const std::string src = "print(\"hello\");\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints on print() call");
    });

    // ---- MYT-355: static method call parameter-name hints ----

    harness.addTest("parameter hints on a static method call", []() {
        const std::string src =
            "class App {\n"
            "    public static function pick(int idx): int { return idx; }\n"
            "}\n"
            "int v = App::pick(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "idx:", InlayHintKind::Parameter) == 1,
            "expected 'idx:' hint on App::pick(7)");
    });

    harness.addTest("parameter hints on a static method call with multiple args", []() {
        const std::string src =
            "class App {\n"
            "    public static function place(int x, int y, int z): int {\n"
            "        return x + y + z;\n"
            "    }\n"
            "}\n"
            "int v = App::place(1, 2, 3);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "x:", InlayHintKind::Parameter) == 1,
            "expected 'x:' hint on App::place");
        require(countHintsWithLabel(hints, "y:", InlayHintKind::Parameter) == 1,
            "expected 'y:' hint on App::place");
        require(countHintsWithLabel(hints, "z:", InlayHintKind::Parameter) == 1,
            "expected 'z:' hint on App::place");
    });

    harness.addTest("parameter hints on a static method call inside another method body", []() {
        // Nested call site — verifies the classifier still fires from
        // inside an instance-method body and doesn't get confused by
        // the enclosing `function name(...)` declaration.
        const std::string src =
            "class App {\n"
            "    public static function pick(int idx): int { return idx; }\n"
            "}\n"
            "class Other {\n"
            "    public constructor() {}\n"
            "    public function run(): void {\n"
            "        int v = App::pick(7);\n"
            "    }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "idx:", InlayHintKind::Parameter) == 1,
            "expected 'idx:' hint on nested App::pick call");
    });

    harness.addTest("bare-identifier suppression still applies to static calls", []() {
        // LSP "no hint where it duplicates source text" rule must
        // suppress the hint when the arg lexeme matches the param name.
        const std::string src =
            "class App {\n"
            "    public static function pick(int idx): int { return idx; }\n"
            "}\n"
            "int idx = 5;\n"
            "int v = App::pick(idx);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "idx:", InlayHintKind::Parameter) == 0,
            "expected 'idx:' hint suppressed when arg matches param name");
    });

    harness.addTest("unknown class on a static call produces no hint and no crash", []() {
        const std::string src = "int v = Unknown::foo(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        // The require() helper surfaces any thrown exception as a
        // failure, so the absence of try/catch IS the no-crash check.
        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints for unknown class on static call");
    });

    harness.addTest("known class but unknown static method produces no hint and no crash", []() {
        const std::string src =
            "class App {\n"
            "    public static function pick(int idx): int { return idx; }\n"
            "}\n"
            "int v = App::missing(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints when static method is undefined");
    });

    // ---- MYT-356: interface-typed receiver parameter hints ----

    harness.addTest("parameter hints on a method call with interface-typed receiver", []() {
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "Function f = x -> x * 2;\n"
            "int v = f.apply(3);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "x:", InlayHintKind::Parameter) == 1,
            "expected 'x:' hint on interface-typed receiver call");
    });

    harness.addTest("parameter hints on multi-arg interface method", []() {
        const std::string src =
            "interface BinaryFunction {\n"
            "    function apply(int a, int b): int;\n"
            "}\n"
            "BinaryFunction bf = (a, b) -> a + b;\n"
            "int v = bf.apply(7, 9);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "a:", InlayHintKind::Parameter) == 1,
            "expected 'a:' hint on multi-arg interface call");
        require(countHintsWithLabel(hints, "b:", InlayHintKind::Parameter) == 1,
            "expected 'b:' hint on multi-arg interface call");
    });

    harness.addTest("parameter hints on a multi-method interface (lookup by name)", []() {
        // Two methods in the interface — name scan must pick the right
        // signature, not just the first one declared.
        const std::string src =
            "interface Reactor {\n"
            "    function reset(): void;\n"
            "    function apply(int x): int;\n"
            "}\n"
            "Reactor r = (x) -> x;\n"
            "int v = r.apply(5);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "x:", InlayHintKind::Parameter) == 1,
            "expected 'x:' hint resolved by name scan on multi-method interface");
    });

    harness.addTest("bare-identifier suppression still applies to interface method calls", []() {
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "Function f = x -> x * 2;\n"
            "int x = 4;\n"
            "int v = f.apply(x);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "x:", InlayHintKind::Parameter) == 0,
            "expected 'x:' hint suppressed when arg lexeme matches param name");
    });

    harness.addTest("class method wins over interface fallback (regression)", []() {
        // Class implements interface AND defines its own version with
        // a different param name. The class path must be consulted
        // first — the interface fallback must not shadow it.
        const std::string src =
            "interface Runnable {\n"
            "    function run(int q): int;\n"
            "}\n"
            "class Job implements Runnable {\n"
            "    public constructor() {}\n"
            "    public function run(int n): int { return n; }\n"
            "}\n"
            "Job job = new Job();\n"
            "int v = job.run(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "n:", InlayHintKind::Parameter) == 1,
            "expected class-side 'n:' hint to win over interface-side 'q:'");
        require(countHintsWithLabel(hints, "q:", InlayHintKind::Parameter) == 0,
            "interface-side 'q:' must not appear when class defines the method");
    });

    harness.addTest("unknown receiver type produces no hint and no crash", []() {
        // Receiver's declared type isn't in the class registry or the
        // interface registry — both lookups miss; result is empty.
        const std::string src =
            "Nowhere n = new Nowhere();\n"
            "int v = n.apply(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints for unknown receiver type");
    });

    // ---- MYT-363: chained method-call parameter-name hints ----

    harness.addTest("parameter hints on the second link of a chained method call", []() {
        // `s.pipe(1).tap(2)` — without MYT-363 only `n:` shows; the
        // `.tap(...)` link is dropped because its receiver is the
        // closing paren of the prior call, not a bare identifier.
        const std::string src =
            "class S {\n"
            "    public constructor() {}\n"
            "    public function pipe(int n): S { return this; }\n"
            "    public function tap(int delta): void {}\n"
            "}\n"
            "S s = new S();\n"
            "s.pipe(1).tap(2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "n:", InlayHintKind::Parameter) == 1,
            "expected 'n:' hint on the head call");
        require(countHintsWithLabel(hints, "delta:", InlayHintKind::Parameter) == 1,
            "expected 'delta:' hint on the chained `.tap(...)` link");
    });

    harness.addTest("parameter hints on every link of a three-deep chain", []() {
        // Verifies the return-type carry-through is iterative: pipe's
        // return → pipe's return → tap's receiver class, all resolved
        // off the closed RPAREN of the preceding link.
        const std::string src =
            "class S {\n"
            "    public constructor() {}\n"
            "    public function pipe(int n): S { return this; }\n"
            "    public function tap(int delta): void {}\n"
            "}\n"
            "S s = new S();\n"
            "s.pipe(1).pipe(2).tap(3);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "n:", InlayHintKind::Parameter) == 2,
            "expected two 'n:' hints, one for each `.pipe(...)` link");
        require(countHintsWithLabel(hints, "delta:", InlayHintKind::Parameter) == 1,
            "expected 'delta:' hint on the trailing `.tap(...)` link");
    });

    harness.addTest("parameter hints on a chain following a static call", []() {
        // Mirrors the Jira repro shape: `Streams::of(elements).filter(predicate)`.
        // Exercises the StaticMethod return-type path feeding the
        // chained-link classifier.
        const std::string src =
            "class Streams {\n"
            "    public constructor() {}\n"
            "    public static function of(int elements): Streams { return new Streams(); }\n"
            "    public function filter(int predicate): Streams { return this; }\n"
            "}\n"
            "Streams x = Streams::of(5).filter(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "elements:", InlayHintKind::Parameter) == 1,
            "expected 'elements:' hint on Streams::of(5)");
        require(countHintsWithLabel(hints, "predicate:", InlayHintKind::Parameter) == 1,
            "expected 'predicate:' hint on the chained .filter(7)");
    });

    harness.addTest("chained call with unresolvable trailing method degrades cleanly", []() {
        // The head call resolves and its return type (S) is recorded,
        // but `.missing(...)` doesn't exist on S — emitParameterHints
        // gets an empty paramNames vector and emits nothing for the
        // trailing link. No crash, no spurious hints, head still hinted.
        const std::string src =
            "class S {\n"
            "    public constructor() {}\n"
            "    public function pipe(int n): S { return this; }\n"
            "}\n"
            "S s = new S();\n"
            "s.pipe(1).missing(2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "n:", InlayHintKind::Parameter) == 1,
            "expected 'n:' hint still emitted on resolvable head call");
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 1,
            "expected exactly 1 Parameter hint (trailing link is unresolved)");
    });

    harness.addTest("chained call through an interface-typed return resolves", []() {
        // Head method returns an interface; the trailing `.tap(...)` is
        // resolved via the interface-method fallback on the return-type
        // class. Guards the class→interface fallback on the return-type
        // side (mirror of the receiver-side fallback at MYT-356).
        const std::string src =
            "interface Pipe {\n"
            "    function tap(int delta): void;\n"
            "}\n"
            "class PipeImpl implements Pipe {\n"
            "    public constructor() {}\n"
            "    public function tap(int delta): void {}\n"
            "}\n"
            "class S {\n"
            "    public constructor() {}\n"
            "    public function getPipe(int seed): Pipe { return new PipeImpl(); }\n"
            "}\n"
            "S s = new S();\n"
            "s.getPipe(1).tap(2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "seed:", InlayHintKind::Parameter) == 1,
            "expected 'seed:' hint on the head call");
        require(countHintsWithLabel(hints, "delta:", InlayHintKind::Parameter) == 1,
            "expected 'delta:' hint resolved via interface fallback on chained link");
    });

    harness.addTest("bare-identifier suppression still applies on a chained link", []() {
        // The chained-receiver code path must still feed through
        // argumentDuplicatesParamName — a chained `.tap(delta)` where
        // `delta` is a local of that name should drop the hint.
        const std::string src =
            "class S {\n"
            "    public constructor() {}\n"
            "    public function pipe(int n): S { return this; }\n"
            "    public function tap(int delta): void {}\n"
            "}\n"
            "S s = new S();\n"
            "int delta = 5;\n"
            "s.pipe(1).tap(delta);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsWithLabel(hints, "n:", InlayHintKind::Parameter) == 1,
            "expected 'n:' hint on head call (arg is literal, not bare ident)");
        require(countHintsWithLabel(hints, "delta:", InlayHintKind::Parameter) == 0,
            "expected 'delta:' hint suppressed on chained link (arg lexeme matches)");
    });

    // ---- Lambda type hints ----

    harness.addTest("type hint on a lambda parameter without annotation", []() {
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "Function f = x -> x * 2;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        // Expect at least one Type hint on the lambda param. The exact
        // type label depends on mType's primitive toString ("int"), but
        // we assert kind+presence rather than the exact string in case
        // the type renderer changes (e.g., to "Int" if primitives box).
        require(countHintsOfKind(hints, InlayHintKind::Type) == 1,
            "expected exactly 1 Type hint for lambda param");
    });

    harness.addTest("no type hint when lambda parameter is explicitly typed", []() {
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "Function f = (int x) -> x * 2;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 0,
            "expected 0 Type hints when lambda param has explicit type");
    });

    // ---- MYT-354: return-position lambda type hints ----

    harness.addTest("type hint on return-position lambda (single param)", []() {
        // Lambda appears as the return expression; target interface
        // is the enclosing function's declared return type.
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "function make(): Function {\n"
            "    return x -> x * 2;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 1,
            "expected 1 Type hint on return-position lambda param");
    });

    harness.addTest("type hints on return-position lambda (multi-param)", []() {
        const std::string src =
            "interface BinaryFunction {\n"
            "    function apply(int a, int b): int;\n"
            "}\n"
            "function make(): BinaryFunction {\n"
            "    return (a, b) -> a + b;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 2,
            "expected 2 Type hints on return-position multi-param lambda");
    });

    harness.addTest("type hint on block-body return-position lambda", []() {
        // Block-body lambda — the inner `return temp;` is past the
        // walk-back start point, so it doesn't confuse target resolution.
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "function make(): Function {\n"
            "    return x -> {\n"
            "        int temp = x * 2;\n"
            "        return temp + 1;\n"
            "    };\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 1,
            "expected 1 Type hint on block-body return-position lambda");
    });

    harness.addTest("type hint on return-position lambda inside nested block", []() {
        // RETURN sits inside an `if` body; brace-balancing must walk
        // out through the if scope to reach the enclosing function decl.
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "function make(bool flag): Function {\n"
            "    if (flag) {\n"
            "        return x -> x * 2;\n"
            "    }\n"
            "    return x -> x;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        // Both `return x -> ...` sites should get a hint.
        require(countHintsOfKind(hints, InlayHintKind::Type) == 2,
            "expected 2 Type hints (one per nested-block return lambda)");
    });

    harness.addTest("no type hint when enclosing function returns void", []() {
        const std::string src =
            "function make(): void {\n"
            "    return x -> x;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 0,
            "expected 0 Type hints for void-returning enclosing function");
    });

    harness.addTest("no type hint when enclosing function returns non-functional type", []() {
        // `int` is not an interface — findInterface() yields nothing and
        // emitLambdaTypeHints short-circuits.
        const std::string src =
            "function make(): int {\n"
            "    return x -> x;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 0,
            "expected 0 Type hints when return type isn't a functional interface");
    });

    harness.addTest("assign-path lambda inside a function body still resolves (regression)", []() {
        // The ASSIGN walk-back must win for `Function g = x -> x;`
        // even when it sits inside a function whose return type is
        // also a functional interface (MYT-354 must not regress the
        // pre-existing ASSIGN path).
        const std::string src =
            "interface Function {\n"
            "    function apply(int x): int;\n"
            "}\n"
            "function make(): Function {\n"
            "    Function g = x -> x;\n"
            "    return g;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 1,
            "expected 1 Type hint (ASSIGN path) on the assigned lambda");
    });

    harness.addTest("no type hint on a plain typed variable declaration", []() {
        // Sanity check: mType always requires an explicit type on
        // variable declarations, so even though there's no `var` keyword
        // to suppress, the handler must not emit Type hints for these.
        const std::string src = "int x = 5;\nstring s = \"hi\";\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Type) == 0,
            "expected 0 Type hints for explicitly-typed variable decls");
    });

    // ---- Robustness ----

    harness.addTest("returns empty for malformed source without throwing", []() {
        const std::string src = "function (((  \n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        // The require() helper would surface any thrown exception as a
        // test failure, so the absence of an explicit try/catch here IS
        // the assertion that handleInlayHint does not throw on bad input.
        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(hints.empty(),
            "expected empty hint list on malformed source, got "
            + std::to_string(hints.size()));
    });

    harness.addTest("range filter restricts hints to the requested window", []() {
        const std::string src =
            "function draw(int width, int height): void { }\n"
            "draw(10, 20);\n"
            "draw(30, 40);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        // Range covers only line 2 (`draw(30, 40);`). The first call
        // on line 1 must be filtered out.
        Range r;
        r.start.line = 2; r.start.character = 0;
        r.end.line = 2; r.end.character = 100;
        auto hints = handler.handleInlayHint("file:///t.mt", r);

        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 2,
            "expected 2 Parameter hints for the in-range call, got "
            + std::to_string(countHintsOfKind(hints, InlayHintKind::Parameter)));
        // Every emitted hint should sit on line 2.
        for (const auto& h : hints) {
            require(h.position.line == 2,
                "all hints must be on line 2; got line "
                + std::to_string(h.position.line));
        }
    });

    harness.addTest("unresolved callee produces no hints and no crash", []() {
        // `mystery` is not defined; the handler must silently skip the
        // call rather than emit garbage or throw.
        const std::string src = "mystery(1, 2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        require(countHintsOfKind(hints, InlayHintKind::Parameter) == 0,
            "expected 0 Parameter hints for unresolved callee");
    });

    harness.addTest("unknown URI returns empty", []() {
        auto docMgr = makeDocManager("file:///t.mt", "int x = 1;\n");
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///does-not-exist.mt", fullDoc());
        require(hints.empty(),
            "expected empty hints for unknown URI, got "
            + std::to_string(hints.size()));
    });

    harness.addTest("nested call args do not get spurious bare-id suppression", []() {
        // The arg expression `g(width)` is multi-token, so a hint MUST
        // still be emitted at the call to `draw`. Regression guard for
        // the LPAREN-token-counting fix.
        const std::string src =
            "function g(int v): int { return v + 1; }\n"
            "function draw(int width, int height): void { }\n"
            "int width = 5;\n"
            "draw(g(width), 20);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        InlayHintHandler handler(docMgr.get());

        auto hints = handler.handleInlayHint("file:///t.mt", fullDoc());
        // draw's first arg is g(width) — a nested call, NOT a bare 'width'.
        // draw's second arg is 20 — a literal. Both should get hints.
        require(countHintsWithLabel(hints, "width:", InlayHintKind::Parameter) >= 1,
            "expected 'width:' hint for the nested-call arg");
        require(countHintsWithLabel(hints, "height:", InlayHintKind::Parameter) == 1,
            "expected 'height:' hint for literal arg");
    });
}

} // namespace mtype::lsp::test
