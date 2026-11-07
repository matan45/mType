#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace ast { class ASTNode; }

// Namespace stays `mtype::lsp` (not `mtype::lsp::analysis`), like
// AstPositionIndex.hpp, so DocumentManager.cpp's `::analysis::` engine
// references are not shadowed.
namespace mtype::lsp {

/**
 * What one document's AST says about type names (MYT-440): the names it
 * uses in checked type positions, and the names it declares as types,
 * generic parameters or values. UnresolvedTypeAnalyzer subtracts what is
 * known from `referenced`.
 */
struct TypeReferences {
    std::unordered_set<std::string> referenced;    // used in a checked type position
    std::unordered_set<std::string> declaredTypes; // classes and interfaces in this document
    std::unordered_set<std::string> genericParams; // <T> of any class, interface, method or function
    std::unordered_set<std::string> valueNames;    // variables, parameters, fields, methods, functions
};

/**
 * Walks a parsed document and collects its TypeReferences.
 *
 * Checked type positions: local and field declarations, parameter and
 * return types, `new`, array creation, extends / implements / interface
 * extends, catch, for-each, lambda parameters, the head of a static access
 * (`Foo::bar()`, `Foo::X`), and generic arguments, including nested ones.
 * Casts, `instanceof` and generic bounds are not checked.
 *
 * Generic parameters are collected for the whole document rather than per
 * scope: a parameter name used outside its scope is then merely not
 * flagged, never flagged wrongly.
 */
class TypeReferenceCollector {
public:
    static TypeReferences collect(const std::vector<std::unique_ptr<ast::ASTNode>>& roots);

    /**
     * The class names in a type expression: "Map<String, List<Foo>>[]?"
     * gives {"Map", "String", "List", "Foo"}. Qualified names ("Mod::Foo",
     * "Outer.Inner") and anything starting with a digit are dropped; the
     * analysis can't tell what their qualifier is, so it doesn't guess.
     */
    static std::vector<std::string> splitTypeNames(const std::string& typeExpr);
};

} // namespace mtype::lsp
