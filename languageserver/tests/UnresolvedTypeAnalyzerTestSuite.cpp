#include "UnresolvedTypeAnalyzerTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/utils/LspDiagnosticConverter.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "../src/utils/UriUtils.hpp"
#include "TempDir.hpp"

#include <string>
#include <vector>

namespace mtype::lsp::test {

namespace {

constexpr const char* kUri = "file:///test/unresolved.mt";

// The MT-E1003 diagnostics of a parsed document, in the LSP form the
// editor receives (0-based ranges plus the data blob).
std::vector<Diagnostic> unresolvedOf(const Document& doc) {
    std::vector<Diagnostic> out;
    for (const auto& core : doc.diagnostics) {
        if (core.code && std::string(core.code->id) == "MT-E1003") {
            out.push_back(toLspDiagnostic(core, doc.uri));
        }
    }
    return out;
}

std::vector<Diagnostic> analyzeSource(const std::string& source) {
    DocumentManager docMgr;
    docMgr.openDocument(kUri, source, 1);
    return unresolvedOf(*docMgr.getDocument(kUri));
}

std::string describe(const std::vector<Diagnostic>& diags) {
    std::string text;
    for (const auto& d : diags) {
        const std::string name = d.data && d.data->contains("missingName")
            ? (*d.data)["missingName"].get<std::string>() : "?";
        text += " " + name + "@" + std::to_string(d.range.start.line) + ":"
              + std::to_string(d.range.start.character);
    }
    return text.empty() ? " (none)" : text;
}

// One diagnostic exactly covering `name` at 0-based (line, character),
// carrying what the MYT-47 quick fix keys on.
void requireAt(const Diagnostic& d, const std::string& name, int line, int character) {
    const int end = character + static_cast<int>(name.size());
    require(d.range.start.line == line && d.range.start.character == character
            && d.range.end.line == line && d.range.end.character == end,
        "expected '" + name + "' at " + std::to_string(line) + ":" + std::to_string(character)
        + "-" + std::to_string(end) + ", got " + std::to_string(d.range.start.line) + ":"
        + std::to_string(d.range.start.character) + "-" + std::to_string(d.range.end.character));
    require(d.code && d.code->get<std::string>() == "MT-E1003", "expected code MT-E1003");
    require(d.severity == 1, "an unresolved type is an error");
    require(d.data && (*d.data)["exceptionType"] == "ClassNotFoundException",
        "data.exceptionType must be ClassNotFoundException");
    require(d.data && (*d.data)["missingName"] == name, "data.missingName must be '" + name + "'");
}

void requireOneAt(const std::string& source, const std::string& name, int line, int character) {
    const auto diags = analyzeSource(source);
    require(diags.size() == 1, "expected exactly one diagnostic, got" + describe(diags));
    requireAt(diags[0], name, line, character);
}

void requireNone(const std::vector<Diagnostic>& diags, const std::string& why) {
    require(diags.empty(), why + "; got" + describe(diags));
}

// Every checked type position, one test each, with the exact token range.
void registerPositionTests(LspTestHarness& harness) {
    harness.addTest("local declaration type", []() {
        requireOneAt("Circle c = null;\n", "Circle", 0, 0);
    });
    harness.addTest("new expression", []() {
        requireOneAt("Object o = new Circle();\n", "Circle", 0, 15);
    });
    harness.addTest("field type", []() {
        requireOneAt("class Holder {\n    public Circle shape;\n}\n", "Circle", 1, 11);
    });
    harness.addTest("extends", []() {
        requireOneAt("class Ring extends Circle {}\n", "Circle", 0, 19);
    });
    harness.addTest("implements", []() {
        requireOneAt("class Ring implements Shape {}\n", "Shape", 0, 22);
    });
    harness.addTest("interface extends", []() {
        requireOneAt("interface Round extends Shape {}\n", "Shape", 0, 24);
    });
    harness.addTest("static call head", []() {
        requireOneAt("Circle::unit();\n", "Circle", 0, 0);
    });
    harness.addTest("static constant head", []() {
        requireOneAt("int r = Circle::RADIUS;\n", "Circle", 0, 8);
    });
    harness.addTest("nested generic argument", []() {
        requireOneAt("class Box<T> {}\nBox<Box<Circle>> b = null;\n", "Circle", 1, 8);
    });
    harness.addTest("catch type", []() {
        requireOneAt("try {\n} catch (Boom e) {\n}\n", "Boom", 1, 9);
    });
    harness.addTest("for-each variable type", []() {
        requireOneAt("function f(Object[] items): void {\n    for (Circle c : items) {\n    }\n}\n",
                     "Circle", 1, 9);
    });
    harness.addTest("lambda parameter type", []() {
        requireOneAt("interface Shaper {\n    function apply(Object c): Object;\n}\n"
                     "Shaper s = (c: Circle) -> c;\n", "Circle", 3, 15);
    });
    harness.addTest("parameter and return types", []() {
        const auto diags = analyzeSource("function area(Circle c): Circle {\n    return c;\n}\n");
        require(diags.size() == 2, "expected parameter and return type, got" + describe(diags));
        requireAt(diags[0], "Circle", 0, 14);
        requireAt(diags[1], "Circle", 0, 25);
    });
    harness.addTest("every occurrence of an unresolved name is flagged", []() {
        // The MYT-445 acceptance line: the declaration and the `new`.
        const auto diags = analyzeSource("Circle c = new Circle();\n");
        require(diags.size() == 2, "expected two diagnostics, got" + describe(diags));
        requireAt(diags[0], "Circle", 0, 0);
        requireAt(diags[1], "Circle", 0, 15);
    });
}

// Names that resolve without a diagnostic, and token contexts that are not
// type references.
void registerExclusionTests(LspTestHarness& harness) {
    harness.addTest("primitives and arrays are not flagged", []() {
        requireNone(analyzeSource("int a = 1;\nfloat[] f = null;\nstring s = \"x\";\nbool b = true;\n"),
            "primitive types need no import");
    });
    harness.addTest("wrapper types are not flagged", []() {
        requireNone(analyzeSource("Int i = null;\nString s = null;\nFloat f = null;\nBool b = null;\n"),
            "wrapper types are excluded by the MYT-440 spec");
    });
    harness.addTest("Object and Promise are builtins", []() {
        requireNone(analyzeSource("Object o = null;\n"
                                  "function async run(): Promise<void> {\n}\n"),
            "compiler builtins need no import");
    });
    harness.addTest("generic parameters, single and multi-letter", []() {
        requireNone(analyzeSource("class Box<T> {\n    public T value;\n}\n"
                                  "class Bag<Item> {\n    public Item item;\n"
                                  "    public function get(): Item {\n        return this.item;\n    }\n}\n"),
            "declared generic parameters are not types to import");
    });
    harness.addTest("a class declared in the document", []() {
        requireNone(analyzeSource("class Circle {}\nCircle c = new Circle();\n"),
            "a local class resolves");
    });
    harness.addTest("Lombok @Builder companion class", []() {
        requireNone(analyzeSource("@Builder\nclass Pizza {\n    private int size;\n}\n"
                                  "PizzaBuilder b = Pizza::builder();\n"),
            "the synthesized PizzaBuilder resolves");
    });
    harness.addTest("member after a static scope is not a type", []() {
        requireNone(analyzeSource("class Modifier {\n    public static int PUBLIC = 1;\n}\n"
                                  "int p = Modifier::PUBLIC;\n"),
            "PUBLIC is a member, not a type");
    });
    harness.addTest("a member access with the same name is not flagged", []() {
        // Circle is unresolved, but `h.Circle` names a member.
        requireOneAt("Circle c = null;\nObject h = null;\nObject v = h.Circle;\n", "Circle", 0, 0);
    });
    harness.addTest("no diagnostics while the document does not parse", []() {
        requireNone(analyzeSource("Circle c = \n"), "analysis only runs on a clean parse");
    });
}

std::vector<Diagnostic> analyzeInDir(TempDir& dir, const std::string& source, bool reparse = false) {
    const std::string uri = UriUtils::filePathToUri((fs::path(dir.path()) / "main.mt").string());
    dir.createFile("main.mt", source);
    DocumentManager docMgr;
    docMgr.openDocument(uri, source, 1);
    if (reparse) docMgr.updateDocument(uri, source, 2); // second parse: import cache hit
    return unresolvedOf(*docMgr.getDocument(uri));
}

void registerImportTests(LspTestHarness& harness) {
    harness.addTest("an imported class is not flagged", []() {
        TempDir dir;
        dir.createFile("shapes/Circle.mt", "class Circle {}\n");
        requireNone(analyzeInDir(dir, "import { Circle } from \"./shapes/Circle.mt\";\n"
                                      "Circle c = new Circle();\n"),
            "Circle is imported");
    });
    harness.addTest("an import alias and its original both resolve", []() {
        TempDir dir;
        dir.createFile("shapes/Circle.mt", "class Circle {}\n");
        requireNone(analyzeInDir(dir, "import { Circle as Round } from \"./shapes/Circle.mt\";\n"
                                      "Round r = null;\nCircle c = null;\n"),
            "aliases resolve like the compiler resolves them");
    });
    harness.addTest("a transitively imported class resolves on every parse", []() {
        TempDir dir;
        dir.createFile("shapes/Base.mt", "class Base {}\n");
        dir.createFile("shapes/Circle.mt", "import * from \"./Base.mt\";\nclass Circle extends Base {}\n");
        const std::string source = "import * from \"./shapes/Circle.mt\";\nBase b = null;\n";
        requireNone(analyzeInDir(dir, source), "first parse");
        requireNone(analyzeInDir(dir, source, /*reparse=*/true), "second parse");
    });
    harness.addTest("an unresolved import turns the analysis off", []() {
        TempDir dir;
        requireNone(analyzeInDir(dir, "import { Nope } from \"./Nope.mt\";\nCircle c = null;\n"),
            "Circle might come from the missing file");
    });
    harness.addTest("a library import turns the analysis off", []() {
        TempDir dir;
        requireNone(analyzeInDir(dir, "import lib \"collections\";\nCircle c = null;\n"),
            "library types are invisible to the language server");
    });
}

} // namespace

void UnresolvedTypeAnalyzerTestSuite::registerTests(LspTestHarness& harness) {
    registerPositionTests(harness);
    registerExclusionTests(harness);
    registerImportTests(harness);
}

} // namespace mtype::lsp::test
