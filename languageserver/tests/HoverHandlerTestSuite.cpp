#include "HoverHandlerTestSuite.hpp"
#include "../src/handlers/HoverHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void HoverHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // HoverHandler precedence: typeInfo from environment > keyword > type > builtin > nullopt
    // Returns nullopt if doc is null or word is empty.

    harness.addTest("hover on keyword 'class' returns description", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        HoverHandler handler(docMgr.get());
        // "class" is at position (0, 0-4), hover at (0, 2) extracts "class"
        auto result = handler.handleHover("file:///test.mt", {0, 2});
        // The word "class" may be extracted as the keyword, or
        // getWordAtPosition may return "class" which then hits the keyword map.
        // However, if the environment resolves typeInfo for "class" first,
        // that takes precedence. Either way, we should get a result.
        require(result.has_value(), "expected hover result for keyword 'class'");
        require(result->contents.find("class") != std::string::npos,
            "hover contents should mention 'class'");
    });

    harness.addTest("hover on type 'int' returns type description", []() {
        auto docMgr = makeDocManager("file:///test.mt", "let x: int = 5;\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 8});
        require(result.has_value(), "expected hover result for type 'int'");
        require(result->contents.find("int") != std::string::npos,
            "hover for 'int' should mention int");
    });

    harness.addTest("hover on builtin 'print' returns signature", []() {
        auto docMgr = makeDocManager("file:///test.mt", "print(42);\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 2});
        require(result.has_value(), "expected hover result for builtin 'print'");
        require(result->contents.find("print") != std::string::npos,
            "hover for 'print' should mention print");
    });

    harness.addTest("hover on unknown URI returns nullopt", []() {
        auto docMgr = makeDocManager("file:///test.mt", "let x: int = 5;\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///nonexistent.mt", {0, 0});
        require(!result.has_value(), "expected nullopt for unknown URI");
    });

    harness.addTest("hover returns nullopt when word is empty", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "   \n", 1);
        HoverHandler handler(&docMgr);
        // All whitespace — getWordAtPosition returns empty
        auto result = handler.handleHover("file:///test.mt", {0, 1});
        require(!result.has_value(), "expected nullopt on whitespace");
    });

    harness.addTest("hover contents are non-empty for known symbols", []() {
        auto docMgr = makeDocManager("file:///test.mt", "return;\n");
        HoverHandler handler(docMgr.get());
        // "return" is in the keyword map
        auto result = handler.handleHover("file:///test.mt", {0, 3});
        require(result.has_value(), "expected hover for keyword 'return'");
        require(!result->contents.empty(), "hover contents should not be empty");
    });

    // MYT-357 — rich hover signatures for user-declared symbols.

    harness.addTest("hover on function call shows full signature with params and return type", []() {
        const std::string src =
            "function add(int a, int b): int { return a + b; }\n"
            "add(1, 2);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // "add" call site at line 1 col 0-2; hover at (1, 1).
        auto result = handler.handleHover("file:///t.mt", {1, 1});
        require(result.has_value(), "expected hover for function 'add'");
        require(result->contents.find("function add(int a, int b)") != std::string::npos,
            "hover should contain full parameter list 'function add(int a, int b)'");
        require(result->contents.find(": int") != std::string::npos,
            "hover should mention return type ': int'");
        require(result->contents.find("```mtype") != std::string::npos,
            "hover should be wrapped in mtype code fence");
    });

    harness.addTest("hover on instance method resolves obj.method to class signature", []() {
        const std::string src =
            "class Foo {\n"
            "    public constructor() {}\n"
            "    public function bar(string s): void {}\n"
            "}\n"
            "Foo f = new Foo();\n"
            "f.bar(\"hi\");\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // "bar" inside `f.bar("hi");` at line 5 cols 2-4; hover at (5, 3).
        auto result = handler.handleHover("file:///t.mt", {5, 3});
        require(result.has_value(), "expected hover for method 'bar'");
        require(result->contents.find("Foo::bar(string s)") != std::string::npos,
            "hover should resolve receiver to 'Foo::bar(string s)'");
        require(result->contents.find("void") != std::string::npos,
            "hover should mention return type 'void'");
    });

    harness.addTest("hover on static method Class::method shows static signature", []() {
        const std::string src =
            "class MyCls {\n"
            "    public static function ping(int n): int { return n; }\n"
            "}\n"
            "MyCls::ping(7);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // "ping" in line 3 at cols 7-10; hover at (3, 8).
        auto result = handler.handleHover("file:///t.mt", {3, 8});
        require(result.has_value(), "expected hover for static method 'ping'");
        require(result->contents.find("static") != std::string::npos,
            "hover should mark method as static");
        require(result->contents.find("MyCls::ping(int n)") != std::string::npos,
            "hover should resolve to 'MyCls::ping(int n)'");
    });

    harness.addTest("hover on constructor in new-expression shows constructor signature", []() {
        const std::string src =
            "class Bar {\n"
            "    public constructor(int x) {}\n"
            "}\n"
            "Bar b = new Bar(5);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'Bar' after 'new ' on line 3 at cols 12-14; hover at (3, 13).
        auto result = handler.handleHover("file:///t.mt", {3, 13});
        require(result.has_value(), "expected hover for constructor 'Bar'");
        require(result->contents.find("Bar(int x)") != std::string::npos,
            "hover should show constructor 'Bar(int x)'");
        require(result->contents.find("function") == std::string::npos,
            "constructor hover should not contain the word 'function'");
    });

    harness.addTest("hover on class name shows full class summary", []() {
        const std::string src =
            "class Box {\n"
            "    public int v;\n"
            "    public function get(): int { return v; }\n"
            "}\n"
            "Box b = new Box();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // First 'Box' on line 4 (not after `new`) cols 0-2; hover at (4, 1).
        auto result = handler.handleHover("file:///t.mt", {4, 1});
        require(result.has_value(), "expected hover for class 'Box'");
        require(result->contents.find("class Box") != std::string::npos,
            "hover should contain header 'class Box'");
        require(result->contents.find("v: int") != std::string::npos,
            "hover should list instance field 'v: int'");
        require(result->contents.find("Box::get()") != std::string::npos,
            "hover should list method 'Box::get()'");
    });

    harness.addTest("hover on interface name shows interface summary with method signatures", []() {
        const std::string src =
            "interface Comparable {\n"
            "    function compareTo(int other): int;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'Comparable' on line 0 at cols 10-19; hover at (0, 15).
        auto result = handler.handleHover("file:///t.mt", {0, 15});
        require(result.has_value(), "expected hover for interface 'Comparable'");
        require(result->contents.find("interface Comparable") != std::string::npos,
            "hover should contain 'interface Comparable'");
        require(result->contents.find("compareTo") != std::string::npos,
            "hover should list abstract method 'compareTo'");
    });

    harness.addTest("hover on overloaded function lists all overloads", []() {
        const std::string src =
            "function ovr(int x): int { return x; }\n"
            "function ovr(string s): int { return 0; }\n"
            "ovr(42);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'ovr' call on line 2 at cols 0-2; hover at (2, 1).
        auto result = handler.handleHover("file:///t.mt", {2, 1});
        require(result.has_value(), "expected hover for overloaded function 'ovr'");
        require(result->contents.find("function ovr(int x)") != std::string::npos,
            "hover should list int overload");
        require(result->contents.find("function ovr(string s)") != std::string::npos,
            "hover should list string overload");
    });

    harness.addTest("hover on unknown identifier returns nullopt", []() {
        const std::string src =
            "function known(): int { return 0; }\n"
            "// nonExistentThing here\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'nonExistentThing' is inside a comment; not in any registry, not a
        // keyword/type/builtin. extractWordAtPosition still returns it.
        auto result = handler.handleHover("file:///t.mt", {1, 10});
        require(!result.has_value(),
            "expected nullopt for identifier not in environment or fallback maps");
    });

    // MYT-357 — regression: SymbolRegistrationVisitor lost class names on
    // method parameters and return types, so hover rendered them as "void buf"
    // and ": object". Verify class names round-trip end-to-end.

    harness.addTest("hover renders class name for object-typed method parameter", []() {
        const std::string src =
            "class SoundBuffer {\n"
            "    public constructor() {}\n"
            "}\n"
            "class Sounds {\n"
            "    public static function create(SoundBuffer buf): void {}\n"
            "}\n"
            "Sounds::create(new SoundBuffer());\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'create' on line 6 (`Sounds::create(...)`) cols 8-13; hover at (6, 10).
        auto result = handler.handleHover("file:///t.mt", {6, 10});
        require(result.has_value(), "expected hover for 'create'");
        require(result->contents.find("SoundBuffer buf") != std::string::npos,
            "param type should render as 'SoundBuffer buf', not 'void buf'");
        require(result->contents.find("void buf") == std::string::npos,
            "param type must not collapse to 'void'");
    });

    harness.addTest("hover renders class name for object-typed method return", []() {
        const std::string src =
            "class Sound {\n"
            "    public constructor() {}\n"
            "}\n"
            "class Factory {\n"
            "    public static function make(): Sound { return new Sound(); }\n"
            "}\n"
            "Factory::make();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'make' on line 6 (`Factory::make();`) cols 9-12; hover at (6, 10).
        auto result = handler.handleHover("file:///t.mt", {6, 10});
        require(result.has_value(), "expected hover for 'make'");
        require(result->contents.find(": Sound") != std::string::npos,
            "return type should render as ': Sound', not ': object'");
        require(result->contents.find(": object") == std::string::npos,
            "return type must not collapse to 'object'");
    });

    harness.addTest("hover renders class name for object-typed function return", []() {
        const std::string src =
            "class Widget {\n"
            "    public constructor() {}\n"
            "}\n"
            "function build(): Widget { return new Widget(); }\n"
            "build();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'build' call on line 4 (`build();`) cols 0-4; hover at (4, 2).
        auto result = handler.handleHover("file:///t.mt", {4, 2});
        require(result.has_value(), "expected hover for 'build'");
        require(result->contents.find(": Widget") != std::string::npos,
            "function return type should render as ': Widget'");
        require(result->contents.find(": object") == std::string::npos,
            "function return type must not collapse to 'object'");
    });

    harness.addTest("hover resolves Class::field.method() chain via static field type", []() {
        const std::string src =
            "class Helper {\n"
            "    public constructor() {}\n"
            "    public function ping(): void {}\n"
            "}\n"
            "class App {\n"
            "    public static Helper h = new Helper();\n"
            "}\n"
            "App::h.ping();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        HoverHandler handler(docMgr.get());
        // 'ping' on line 7 (`App::h.ping();`) cols 7-10; hover at (7, 8).
        auto result = handler.handleHover("file:///t.mt", {7, 8});
        require(result.has_value(), "expected hover for 'ping' via Class::field.method()");
        require(result->contents.find("Helper::ping()") != std::string::npos,
            "method should resolve via static field's class to 'Helper::ping()'");
    });
}

} // namespace mtype::lsp::test
