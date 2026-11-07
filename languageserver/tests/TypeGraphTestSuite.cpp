#include "TypeGraphTestSuite.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/UriUtils.hpp"
#include "TempDir.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

namespace mtype::lsp::test {

namespace {

using analysis::TypeDecl;
using analysis::TypeMember;

std::string uriOf(const TempDir& dir, const std::string& relativePath) {
    return UriUtils::filePathToUri(
        (std::filesystem::path(dir.path()) / relativePath).string());
}

std::string joined(const std::vector<std::string>& names) {
    std::string out = "[";
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i) out += ", ";
        out += names[i];
    }
    return out + "]";
}

void requireNames(const std::vector<TypeDecl>& decls,
                  const std::vector<std::string>& expected, const std::string& what) {
    std::vector<std::string> actual;
    for (const auto& d : decls) actual.push_back(d.name);
    require(actual == expected, what + ": expected " + joined(expected) + ", got " + joined(actual));
}

TypeDecl onlyType(const analysis::WorkspaceTypeGraph& graph, const std::string& name) {
    const auto found = graph.findTypes(name);
    require(found.size() == 1, "expected one '" + name + "', got " + std::to_string(found.size()));
    return found[0];
}

const TypeMember* memberNamed(const std::vector<TypeMember>& members,
                              const std::string& name, std::size_t arity = 0) {
    for (const auto& m : members) {
        if (m.name == name && m.arity == arity) return &m;
    }
    return nullptr;
}

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

// The `n` characters of `source` starting at `p`.
std::string textFrom(const std::string& source, const Position& p, std::size_t n) {
    const auto lines = linesOf(source);
    if (p.line < 0 || p.line >= static_cast<int>(lines.size())) return "<no line>";
    const auto& line = lines[p.line];
    if (p.character < 0 || static_cast<std::size_t>(p.character) > line.size()) return "<no column>";
    return line.substr(p.character, n);
}

// The text a single-line range covers.
std::string textAt(const std::string& source, const Range& r) {
    if (r.start.line != r.end.line || r.end.character < r.start.character) return "<not one line>";
    return textFrom(source, r.start, static_cast<std::size_t>(r.end.character - r.start.character));
}

// The last character a range ending at `p` covers.
std::string lastCharBefore(const std::string& source, const Position& p) {
    if (p.character < 1) return "<start of line>";
    return textFrom(source, Position{p.line, p.character - 1}, 1);
}

bool sameRange(const Range& a, const Range& b) {
    return a.start.line == b.start.line && a.start.character == b.start.character
        && a.end.line == b.end.line && a.end.character == b.end.character;
}

// Runs `fn` on its own thread so a non-terminating walk fails the test
// instead of hanging the run. `fn` must own what it touches.
template <typename Fn>
bool finishesWithin(std::chrono::milliseconds limit, Fn fn) {
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    std::thread([done, fn]() mutable {
        fn();
        done->set_value();
    }).detach();
    return future.wait_for(limit) == std::future_status::ready;
}

void registerEdgeTests(LspTestHarness& harness) {

    harness.addTest("interface -> class edge from closed files", []() {
        TempDir root;
        root.createFile("Animal.mt", "interface Animal {\n    function speak(): void;\n}\n");
        root.createFile("Dog.mt",
            "import { Animal } from \"./Animal.mt\";\n"
            "class Dog implements Animal {\n"
            "    public function speak(): void {}\n"
            "}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        requireNames(graph.directSubtypes("Animal"), {"Dog"}, "subtypes of Animal");
        requireNames(graph.directSupertypes("Dog"), {"Animal"}, "supertypes of Dog");
        const auto dog = onlyType(graph, "Dog");
        require(dog.kind == analysis::TypeDeclKind::Class, "Dog is a class");
        require(dog.supertypes.size() == 1
            && dog.supertypes[0].relation == analysis::SupertypeRelation::Implements,
            "Dog has one Implements edge");
        require(dog.supertypes[0].uriHint == uriOf(root, "Animal.mt"),
            "hint should name Animal.mt, got '" + dog.supertypes[0].uriHint + "'");
    });

    harness.addTest("interface extends interface", []() {
        TempDir root;
        root.createFile("Named.mt", "interface Named {}\n");
        root.createFile("Pet.mt", "import { Named } from \"./Named.mt\";\ninterface Pet extends Named {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());

        const auto pet = onlyType(index.typeGraph(), "Pet");
        require(pet.kind == analysis::TypeDeclKind::Interface, "Pet is an interface");
        require(pet.supertypes.size() == 1 && pet.supertypes[0].name == "Named"
            && pet.supertypes[0].relation == analysis::SupertypeRelation::Extends,
            "Pet extends Named");
        requireNames(index.typeGraph().directSubtypes("Named"), {"Pet"}, "subtypes of Named");
    });

    harness.addTest("generic parents are keyed by their base name", []() {
        TempDir root;
        root.createFile("Box.mt", "class Box<T> {}\n");
        root.createFile("Cmp.mt", "interface Cmp<T> {}\n");
        root.createFile("IntBox.mt",
            "import { Box } from \"./Box.mt\";\n"
            "import { Cmp } from \"./Cmp.mt\";\n"
            "class IntBox extends Box<Item> implements Cmp<IntBox> {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        const auto intBox = onlyType(graph, "IntBox");
        require(intBox.supertypes.size() == 2, "IntBox has two edges");
        require(intBox.supertypes[0].name == "Box" && intBox.supertypes[0].spelling == "Box<Item>",
            "extends edge: got '" + intBox.supertypes[0].name + "' / '" + intBox.supertypes[0].spelling + "'");
        require(intBox.supertypes[1].name == "Cmp" && intBox.supertypes[1].spelling == "Cmp<IntBox>",
            "implements edge: got '" + intBox.supertypes[1].name + "' / '" + intBox.supertypes[1].spelling + "'");
        require(onlyType(graph, "Box").genericParams == std::vector<std::string>{"T"}, "Box<T> keeps T");
        requireNames(graph.directSubtypes("Box"), {"IntBox"}, "subtypes of Box");
        requireNames(graph.directSubtypes("Cmp"), {"IntBox"}, "subtypes of Cmp");
    });

    harness.addTest("an import alias maps back to the original type", []() {
        TempDir root;
        root.createFile("Comparable.mt", "interface Comparable<T> {}\n");
        root.createFile("Dog.mt",
            "import { Comparable as Cmp } from \"./Comparable.mt\";\n"
            "class Dog implements Cmp<Dog> {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        requireNames(graph.directSubtypes("Comparable"), {"Dog"}, "subtypes of Comparable");
        require(graph.directSubtypes("Cmp").empty(), "the alias is not a type of its own");
        require(onlyType(graph, "Dog").supertypes[0].spelling == "Cmp<Dog>", "the spelling is kept");
    });

    harness.addTest("transitive subtypes list each type once", []() {
        TempDir root;
        root.createFile("Shape.mt", "interface Shape {}\n");
        root.createFile("Polygon.mt", "import { Shape } from \"./Shape.mt\";\ninterface Polygon extends Shape {}\n");
        root.createFile("Quad.mt",
            "import { Shape } from \"./Shape.mt\";\n"
            "import { Polygon } from \"./Polygon.mt\";\n"
            "class Quad implements Polygon, Shape {}\n");
        root.createFile("Square.mt", "import { Quad } from \"./Quad.mt\";\nclass Square extends Quad {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        requireNames(graph.directSubtypes("Shape"), {"Polygon", "Quad"}, "direct subtypes of Shape");
        requireNames(graph.transitiveSubtypes("Shape"), {"Polygon", "Quad", "Square"},
            "transitive subtypes of Shape");
        requireNames(graph.transitiveSubtypes("Square"), {}, "a leaf has no subtypes");
    });

    harness.addTest("a cycle across files terminates and excludes the root", []() {
        TempDir root;
        root.createFile("Alpha.mt", "import { Beta } from \"./Beta.mt\";\ninterface Alpha extends Beta {}\n");
        root.createFile("Beta.mt", "import { Alpha } from \"./Alpha.mt\";\ninterface Beta extends Alpha {}\n");
        auto index = std::make_shared<analysis::WorkspaceSymbolIndex>();
        index->buildFromWorkspace(root.path());

        auto result = std::make_shared<std::vector<TypeDecl>>();
        const bool done = finishesWithin(std::chrono::seconds(2), [index, result]() {
            *result = index->typeGraph().transitiveSubtypes("Alpha");
        });
        require(done, "transitiveSubtypes must terminate on a cycle");
        requireNames(*result, {"Beta"}, "transitive subtypes of Alpha");
        requireNames(index->typeGraph().transitiveSubtypes("Beta"), {"Alpha"}, "transitive subtypes of Beta");
    });
}

void registerMemberAndHintTests(LspTestHarness& harness) {

    harness.addTest("members of a closed file, with name and declaration ranges", []() {
        const std::string source =
            "class Dog {\n"
            "    public string name;\n"
            "    public function bark(): void {\n"
            "    }\n"
            "    public function bark(int times): void {}\n"
            "}\n";
        TempDir root;
        root.createFile("Dog.mt", source);
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path()); // never opened

        const auto members = index.typeGraph().membersOf("Dog");
        require(members.size() == 3, "expected 3 members, got " + std::to_string(members.size()));
        const auto* field = memberNamed(members, "name");
        const auto* bark = memberNamed(members, "bark", 0);
        const auto* barkTimes = memberNamed(members, "bark", 1);
        require(field && bark && barkTimes, "expected name, bark() and bark(int) (overloads kept)");

        require(field->kind == analysis::TypeMemberKind::Field && field->type == "string", "name is a string field");
        require(field->ownerUri == uriOf(root, "Dog.mt"), "members carry their file");
        require(textAt(source, field->nameRange) == "name", "field nameRange: '" + textAt(source, field->nameRange) + "'");
        require(lastCharBefore(source, field->declRange.end) == ";", "field declRange ends after ';'");

        require(textAt(source, bark->nameRange) == "bark", "bark nameRange: '" + textAt(source, bark->nameRange) + "'");
        require(textFrom(source, bark->declRange.start, 6) == "public", "bark declRange starts at its modifier");
        require(bark->declRange.end.line == 3 && lastCharBefore(source, bark->declRange.end) == "}",
            "bark declRange ends after its closing brace");
        require(bark->type == "void" && bark->paramTypes.empty(), "bark(): void");
        require(barkTimes->paramTypes == std::vector<std::string>{"int"}, "bark(int times) keeps its param type");
    });

    harness.addTest("interface signatures are abstract public methods", []() {
        const std::string source =
            "interface Shape {\n"
            "    function area(): float;\n"
            "    function scale(float factor): void;\n"
            "}\n";
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///mem/Shape.mt", source);

        const auto members = index.typeGraph().membersOf("Shape");
        const auto* area = memberNamed(members, "area");
        const auto* scale = memberNamed(members, "scale", 1);
        require(members.size() == 2 && area && scale, "expected area() and scale(float)");
        require(area->isAbstract && area->access == ast::AccessModifier::PUBLIC, "signatures are abstract and public");
        require(textAt(source, area->nameRange) == "area", "area nameRange: '" + textAt(source, area->nameRange) + "'");
        require(lastCharBefore(source, area->declRange.end) == ";", "a signature's declRange ends at ';'");
        require(scale->paramTypes == std::vector<std::string>{"float"}, "scale(float) keeps its param type");
    });

    harness.addTest("same-named types in two folders resolve through imports", []() {
        TempDir root;
        root.createFile("zoo/Animal.mt", "interface Animal {}\n");
        root.createFile("zoo/Dog.mt", "import { Animal } from \"./Animal.mt\";\nclass Dog implements Animal {}\n");
        root.createFile("farm/Barn.mt", "interface Animal {}\nclass Cow implements Animal {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();
        const std::string zooAnimal = uriOf(root, "zoo/Animal.mt");
        const std::string barn = uriOf(root, "farm/Barn.mt");

        require(graph.findTypes("Animal").size() == 2, "both Animals are indexed");
        requireNames(graph.directSubtypes("Animal", zooAnimal), {"Dog"}, "subtypes of zoo's Animal");
        requireNames(graph.directSubtypes("Animal", barn), {"Cow"}, "subtypes of farm's Animal");
        requireNames(graph.directSubtypes("Animal"), {"Cow", "Dog"}, "an unscoped query sees both");

        const auto dogParents = graph.directSupertypes("Dog");
        require(dogParents.size() == 1 && dogParents[0].fileUri == zooAnimal, "Dog's parent is zoo's Animal");
        const auto cowParents = graph.directSupertypes("Cow");
        require(cowParents.size() == 1 && cowParents[0].fileUri == barn, "Cow's parent is the Animal beside it");
    });

    harness.addTest("an unresolvable hint falls back to the type name", []() {
        TempDir root;
        root.createFile("lib/Missing.mt", "interface Missing {}\n");
        root.createFile("X.mt", "import { Missing } from \"./Missing.mt\";\nclass X implements Missing {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        requireNames(graph.directSubtypes("Missing", uriOf(root, "lib/Missing.mt")), {"X"},
            "a hint naming no indexed file must not hide X");
        requireNames(graph.transitiveSubtypes("Missing"), {"X"}, "unscoped query");
    });
}

void registerFreshnessTests(LspTestHarness& harness) {

    harness.addTest("graph: an open buffer wins over disk, and closing restores disk", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog extends Wolf {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const std::string uri = uriOf(root, "Dog.mt");
        const auto& graph = index.typeGraph();

        index.openFile(uri, "class Dog extends Fox {}\n");
        index.reindexFile(uri); // file-watcher event: disk still says Wolf
        requireNames(graph.directSubtypes("Fox"), {"Dog"}, "the buffer's edge");
        requireNames(graph.directSubtypes("Wolf"), {}, "disk must not win while open");

        index.revertToDisk(uri);
        requireNames(graph.directSubtypes("Wolf"), {"Dog"}, "closing restores the disk edge");
        requireNames(graph.directSubtypes("Fox"), {}, "the buffer's edge is gone");
    });

    harness.addTest("graph: a parse error keeps the last good edges and members", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///mem/Dog.mt",
            "class Dog extends Wolf {\n    public function run(): void {}\n}\n");
        index.reindexFile("file:///mem/Dog.mt", "class Dog extends Fox {\n"); // unterminated

        requireNames(index.typeGraph().directSubtypes("Wolf"), {"Dog"}, "last good edge kept");
        requireNames(index.typeGraph().directSubtypes("Fox"), {}, "the broken edit is ignored");
        require(memberNamed(index.typeGraph().membersOf("Dog"), "run"), "last good members kept");
    });

    harness.addTest("graph: the workspace scan skips an open buffer", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog extends Wolf {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.openFile(uriOf(root, "Dog.mt"), "class Dog extends Fox {}\n"); // restored dirty tab
        index.buildFromWorkspace(root.path());

        requireNames(index.typeGraph().directSubtypes("Fox"), {"Dog"}, "the buffer survives the scan");
        requireNames(index.typeGraph().directSubtypes("Wolf"), {}, "the disk version is not loaded");
    });

    harness.addTest("graph: replacing or deleting a file drops its old links", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///mem/Dog.mt", "class Dog extends Wolf {}\n");
        index.reindexFile("file:///mem/Dog.mt", "class Cat {}\n");
        const auto& graph = index.typeGraph();

        requireNames(graph.directSubtypes("Wolf"), {}, "Dog's old edge is gone");
        require(graph.findTypes("Dog").empty() && graph.findTypes("Cat").size() == 1, "Cat replaced Dog");
        index.invalidateFile("file:///mem/Dog.mt");
        require(graph.findTypes("Cat").empty(), "a deleted file leaves the graph");
    });

    harness.addTest("graph: Lombok members of closed files, without builder types", []() {
        TempDir root;
        root.createFile("Person.mt", "@Getter\nclass Person {\n    private string name;\n}\n");
        root.createFile("Config.mt", "@Builder\nclass Config {\n    private int port;\n}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const auto& graph = index.typeGraph();

        const auto person = onlyType(graph, "Person");
        const auto personMembers = graph.membersOf("Person");
        const auto* field = memberNamed(personMembers, "name");
        const auto* getter = memberNamed(personMembers, "getName");
        require(field && !field->isSynthetic, "the declared field is a source member");
        require(getter && getter->isSynthetic, "@Getter's getName() is a synthetic member");
        require(sameRange(getter->nameRange, person.nameRange) && sameRange(getter->declRange, person.nameRange),
            "a generated member points at its class's name");

        const auto* builder = memberNamed(graph.membersOf("Config"), "builder");
        require(builder && builder->isSynthetic, "@Builder's builder() is a synthetic member");
        require(graph.findTypes("ConfigBuilder").empty(), "generated builder classes are not types");
        require(index.findByName("ConfigBuilder").empty(), "nor workspace symbols");
    });

    harness.addTest("graph: concurrent queries during reindexing", []() {
        auto index = std::make_shared<analysis::WorkspaceSymbolIndex>();
        index->reindexFile("file:///mem/A.mt", "interface A extends B {}\n");
        index->reindexFile("file:///mem/B.mt", "interface B extends A {}\n");

        std::atomic<bool> go{false};
        std::atomic<bool> failed{false};
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&]() {
                while (!go.load()) {}
                try {
                    for (int j = 0; j < 200; ++j) {
                        (void)index->typeGraph().transitiveSubtypes("A");
                        (void)index->typeGraph().directSupertypes("B");
                        (void)index->typeGraph().membersOf("A");
                    }
                } catch (...) {
                    failed.store(true);
                }
            });
        }
        threads.emplace_back([&]() {
            while (!go.load()) {}
            try {
                for (int j = 0; j < 100; ++j) {
                    index->reindexFile("file:///mem/B.mt",
                        j % 2 ? "interface B {}\n" : "interface B extends A {}\n");
                }
            } catch (...) {
                failed.store(true);
            }
        });
        go.store(true);
        for (auto& t : threads) t.join();

        require(!failed.load(), "concurrent graph access must not throw");
        // The writer's last pass (j = 99) left `interface B {}`: A still
        // extends B, but B no longer extends A.
        requireNames(index->typeGraph().transitiveSubtypes("B"), {"A"}, "final state: A extends B");
        requireNames(index->typeGraph().transitiveSubtypes("A"), {}, "final state: nothing extends A");
    });
}

} // namespace

void TypeGraphTestSuite::registerTests(LspTestHarness& harness) {
    registerEdgeTests(harness);
    registerMemberAndHintTests(harness);
    registerFreshnessTests(harness);
}

} // namespace mtype::lsp::test
