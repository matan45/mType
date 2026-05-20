// MYT-299 — Internal helpers shared between the prepare/incoming/outgoing
// translation units of CallHierarchyHandler. Not a public LSP header — do
// not include from outside src/handlers/CallHierarchy*.cpp.
#pragma once

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../../mType/ast/nodes/classes/ThisConstructorCallNode.hpp"
#include "../../../mType/ast/nodes/classes/NewNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/ast/nodes/expressions/VariableNode.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mtype::lsp::callhier {

namespace nf = ::ast::nodes::functions;
namespace nc = ::ast::nodes::classes;
namespace ns = ::ast::nodes::statements;
namespace ne = ::ast::nodes::expressions;

inline constexpr const char* kKindFunction = "function";
inline constexpr const char* kKindMethod = "method";
inline constexpr const char* kKindConstructor = "constructor";
inline constexpr std::size_t kConstructorKeywordLen = 11;

inline Position toLspPosition(const ::errors::SourceLocation& loc) {
    int line = loc.getLine() - 1;
    int col = loc.getColumn() - 1;
    return Position{std::max(0, line), std::max(0, col)};
}

inline Range tokenRange(const ::errors::SourceLocation& loc, std::size_t length) {
    Position start = toLspPosition(loc);
    return Range{start, Position{start.line, start.character + static_cast<int>(length)}};
}

// Source-text scan that locates an identifier token on a specific line.
// mType AST SourceLocations are inconsistent — FunctionNode/ClassNode/
// MethodCallNode point at the name token, but MethodNode points at the
// pre-modifier start, and FunctionCallNode/NewNode point AFTER `)`. Rather
// than special-case each, we look up the actual name in the source text.
// Returns -1 if not found.
inline int findTokenColumn(const std::string& content, int line0Based,
                            const std::string& name, int startCol0Based) {
    if (name.empty()) return -1;
    int curLine = 0;
    std::size_t lineStart = 0;
    while (curLine < line0Based && lineStart < content.size()) {
        std::size_t nl = content.find('\n', lineStart);
        if (nl == std::string::npos) return -1;
        lineStart = nl + 1;
        ++curLine;
    }
    std::size_t lineEnd = content.find('\n', lineStart);
    if (lineEnd == std::string::npos) lineEnd = content.size();
    std::size_t search = lineStart;
    if (startCol0Based > 0) {
        search = lineStart + static_cast<std::size_t>(startCol0Based);
        if (search > lineEnd) return -1;
    }
    while (search < lineEnd) {
        std::size_t pos = content.find(name, search);
        if (pos == std::string::npos || pos >= lineEnd) return -1;
        char prev = pos > lineStart ? content[pos - 1] : ' ';
        char next = pos + name.size() < lineEnd ? content[pos + name.size()] : ' ';
        bool wordBoundaryPrev = !(std::isalnum(static_cast<unsigned char>(prev)) || prev == '_');
        bool wordBoundaryNext = !(std::isalnum(static_cast<unsigned char>(next)) || next == '_');
        if (wordBoundaryPrev && wordBoundaryNext) {
            return static_cast<int>(pos - lineStart);
        }
        search = pos + 1;
    }
    return -1;
}

// Range for an identifier token whose declaration/call lives at `loc`.
// Falls back to tokenRange(loc, name.size()) if the source scan misses.
inline Range nameTokenRange(const Document* doc, const ::errors::SourceLocation& loc,
                             const std::string& name) {
    if (doc) {
        int line = std::max(0, loc.getLine() - 1);
        int startCol = std::max(0, loc.getColumn() - 1);
        // Search from the location's column first; if not found, retry from
        // the line start (handles MethodNode, NewNode, FunctionCallNode where
        // the location may be on a later token).
        int col = findTokenColumn(doc->content, line, name, startCol);
        if (col < 0) col = findTokenColumn(doc->content, line, name, 0);
        if (col >= 0) {
            return Range{Position{line, col},
                         Position{line, col + static_cast<int>(name.size())}};
        }
    }
    return tokenRange(loc, name.size());
}

inline bool rangeContains(const Range& r, const Position& p) {
    if (p.line < r.start.line || p.line > r.end.line) return false;
    if (p.line == r.start.line && p.character < r.start.character) return false;
    if (p.line == r.end.line && p.character > r.end.character) return false;
    return true;
}

inline ns::ProgramNode* getProgram(const Document* doc) {
    if (!doc) return nullptr;
    for (const auto& n : doc->ast) {
        if (auto* p = dynamic_cast<ns::ProgramNode*>(n.get())) return p;
    }
    return nullptr;
}

inline std::vector<nf::FunctionNode*> collectFunctionsByName(const Document* doc,
                                                              const std::string& name) {
    std::vector<nf::FunctionNode*> out;
    auto* p = getProgram(doc);
    if (!p) return out;
    for (const auto& s : p->getStatements()) {
        if (auto* fn = dynamic_cast<nf::FunctionNode*>(s.get())) {
            if (fn->getName() == name) out.push_back(fn);
        }
    }
    return out;
}

inline std::vector<nc::ClassNode*> collectClasses(const Document* doc) {
    std::vector<nc::ClassNode*> out;
    auto* p = getProgram(doc);
    if (!p) return out;
    for (const auto& s : p->getStatements()) {
        if (auto* c = dynamic_cast<nc::ClassNode*>(s.get())) out.push_back(c);
    }
    return out;
}

inline nc::ClassNode* findClassByName(const Document* doc, const std::string& name) {
    for (auto* c : collectClasses(doc)) {
        if (c->getClassName() == name) return c;
    }
    return nullptr;
}

// Returns the first class matching `name` across every open document. If
// the same class name lives in two open files the earlier-scanned doc
// wins; v1 accepts that collision per Q3 (open-docs-only). A real fix
// requires module/namespace identity, which mType doesn't expose yet.
inline std::pair<nc::ClassNode*, std::string> findClassEverywhere(DocumentManager* mgr,
                                                                  const std::string& name) {
    if (!mgr) return {nullptr, ""};
    for (const auto& uri : mgr->getAllOpenUris()) {
        if (auto* c = findClassByName(mgr->getDocument(uri), name)) return {c, uri};
    }
    return {nullptr, ""};
}

inline std::vector<nc::MethodNode*> collectMethodsByName(nc::ClassNode* cls,
                                                          const std::string& name) {
    std::vector<nc::MethodNode*> out;
    if (!cls) return out;
    for (const auto& m : cls->getMethods()) {
        if (auto* mn = dynamic_cast<nc::MethodNode*>(m.get())) {
            if (mn->getName() == name) out.push_back(mn);
        }
    }
    return out;
}

inline std::vector<nc::ConstructorNode*> collectConstructors(nc::ClassNode* cls) {
    std::vector<nc::ConstructorNode*> out;
    if (!cls) return out;
    for (const auto& c : cls->getConstructors()) {
        if (auto* cn = dynamic_cast<nc::ConstructorNode*>(c.get())) out.push_back(cn);
    }
    return out;
}

// Walk inheritance chain looking for the nearest ancestor that defines
// `methodName`. Returns (ancestor class node, uri) on the first match.
// Returns {nullptr, ""} on no match or broken chain (Q9: no fallback).
inline std::pair<nc::ClassNode*, std::string> resolveSuperMethod(DocumentManager* mgr,
                                                                 const std::string& startClass,
                                                                 const std::string& methodName) {
    std::unordered_set<std::string> visited;
    std::string current = startClass;
    while (!current.empty() && !visited.count(current)) {
        visited.insert(current);
        auto [cls, uri] = findClassEverywhere(mgr, current);
        if (!cls || !cls->hasParentClass()) return {nullptr, ""};
        std::string parent = cls->getParentClassName();
        auto [pcls, puri] = findClassEverywhere(mgr, parent);
        if (!pcls) return {nullptr, ""};
        if (!collectMethodsByName(pcls, methodName).empty()) return {pcls, puri};
        current = parent;
    }
    return {nullptr, ""};
}

// Locate the parent class that owns a constructor across all open docs.
inline std::pair<nc::ClassNode*, std::string> findClassOwningConstructor(
    DocumentManager* mgr, nc::ConstructorNode* target) {
    if (!mgr || !target) return {nullptr, ""};
    for (const auto& uri : mgr->getAllOpenUris()) {
        auto* doc = mgr->getDocument(uri);
        if (!doc) continue;
        for (auto* cls : collectClasses(doc)) {
            for (auto* c : collectConstructors(cls)) {
                if (c == target) return {cls, uri};
            }
        }
    }
    return {nullptr, ""};
}

// Build CallHierarchyItem for a top-level function. Caller passes
// overload count (Q8).
inline CallHierarchyItem itemForFunction(const std::string& uri, nf::FunctionNode* fn,
                                          int overloads) {
    CallHierarchyItem it;
    it.name = fn->getName();
    it.kind = SymbolKind::Function;
    it.uri = uri;
    it.selectionRange = tokenRange(fn->getLocation(), fn->getName().size());
    it.range = it.selectionRange;
    if (overloads > 1) it.detail = std::to_string(overloads) + " overloads";
    json data;
    data["kind"] = kKindFunction;
    data["className"] = "";
    data["name"] = fn->getName();
    it.data = std::move(data);
    return it;
}

inline CallHierarchyItem itemForMethod(const std::string& uri, const std::string& className,
                                       nc::MethodNode* m, int overloads) {
    CallHierarchyItem it;
    it.name = m->getName();
    it.kind = SymbolKind::Method;
    it.uri = uri;
    it.selectionRange = tokenRange(m->getLocation(), m->getName().size());
    it.range = it.selectionRange;
    std::string det = className;
    if (m->getIsStatic()) det += " · static";
    if (overloads > 1) det += " · " + std::to_string(overloads) + " overloads";
    it.detail = std::move(det);
    json data;
    data["kind"] = kKindMethod;
    data["className"] = className;
    data["name"] = m->getName();
    it.data = std::move(data);
    return it;
}

inline CallHierarchyItem itemForConstructor(const std::string& uri, const std::string& className,
                                            nc::ConstructorNode* c, int overloads) {
    CallHierarchyItem it;
    it.name = className;
    it.kind = SymbolKind::Constructor;
    it.uri = uri;
    it.selectionRange = tokenRange(c->getLocation(), kConstructorKeywordLen);
    it.range = it.selectionRange;
    std::string det = "constructor";
    if (overloads > 1) det += " · " + std::to_string(overloads) + " overloads";
    it.detail = std::move(det);
    json data;
    data["kind"] = kKindConstructor;
    data["className"] = className;
    data["name"] = "";
    it.data = std::move(data);
    return it;
}

struct ItemId { std::string kind, className, name; };

inline ItemId extractId(const CallHierarchyItem& item) {
    ItemId id;
    if (!item.data) return id;
    const auto& d = *item.data;
    if (d.contains("kind") && d.at("kind").is_string()) id.kind = d.at("kind").get<std::string>();
    if (d.contains("className") && d.at("className").is_string()) id.className = d.at("className").get<std::string>();
    if (d.contains("name") && d.at("name").is_string()) id.name = d.at("name").get<std::string>();
    return id;
}

// Identity key for grouping by (kind, className, name) per Q8.
inline std::string groupKey(const std::string& kind, const std::string& className,
                            const std::string& name) {
    return kind + "\x1f" + className + "\x1f" + name;
}

// Resolve a call site to one or more target identities (kind/className/name).
// Q2 hybrid + Q9 super chain. enclosingClass = name of the class containing
// the call (empty if the call is in a top-level function).
struct ResolvedTarget {
    std::string kind, className, name;
};
inline std::vector<ResolvedTarget> resolveCall(::ast::ASTNode* call,
                                                DocumentManager* mgr,
                                                const std::string& enclosingClass) {
    std::vector<ResolvedTarget> out;
    if (auto* fc = dynamic_cast<nf::FunctionCallNode*>(call)) {
        // mType parses `ClassName::method(...)` (without generic args) as a
        // FunctionCallNode whose functionName carries the `::` qualifier.
        // Treat it as a class-pinned static method per Q2.
        const std::string& fnName = fc->getFunctionName();
        std::size_t scope = fnName.rfind("::");
        if (scope != std::string::npos) {
            out.push_back({kKindMethod, fnName.substr(0, scope), fnName.substr(scope + 2)});
            return out;
        }
        out.push_back({kKindFunction, "", fnName});
        return out;
    }
    if (auto* nn = dynamic_cast<nc::NewNode*>(call)) {
        out.push_back({kKindConstructor, nn->getClassName(), ""});
        return out;
    }
    if (auto* tcc = dynamic_cast<nc::ThisConstructorCallNode*>(call)) {
        if (!enclosingClass.empty()) {
            out.push_back({kKindConstructor, enclosingClass, ""});
        }
        return out;
    }
    if (auto* scc = dynamic_cast<nc::SuperConstructorCallNode*>(call)) {
        if (!enclosingClass.empty()) {
            auto [cls, uri] = findClassEverywhere(mgr, enclosingClass);
            if (cls && cls->hasParentClass()) {
                out.push_back({kKindConstructor, cls->getParentClassName(), ""});
            }
        }
        return out;
    }
    if (auto* smc = dynamic_cast<nc::SuperMethodCallNode*>(call)) {
        if (!enclosingClass.empty()) {
            auto [anc, uri] = resolveSuperMethod(mgr, enclosingClass, smc->getMethodName());
            if (anc) out.push_back({kKindMethod, anc->getClassName(), smc->getMethodName()});
        }
        return out;
    }
    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(call)) {
        const std::string& name = mc->getMethodName();
        if (mc->getIsStaticCall()) {
            if (auto* vn = dynamic_cast<ne::VariableNode*>(mc->getObject())) {
                out.push_back({kKindMethod, vn->getName(), name});
                return out;
            }
        } else if (auto* vn = dynamic_cast<ne::VariableNode*>(mc->getObject());
                   vn && vn->getName() == "this" && !enclosingClass.empty()) {
            out.push_back({kKindMethod, enclosingClass, name});
            return out;
        }
        // Receiver isn't a bare `this` / class identifier (e.g., chained
        // `getX().y()`, a field access, an indexed expression). Q2 falls
        // back to name-only matching here — no static-type inference in v1.
        if (mgr) {
            for (const auto& uri : mgr->getAllOpenUris()) {
                auto* doc = mgr->getDocument(uri);
                if (!doc) continue;
                for (auto* cls : collectClasses(doc)) {
                    if (!collectMethodsByName(cls, name).empty()) {
                        out.push_back({kKindMethod, cls->getClassName(), name});
                    }
                }
            }
        }
    }
    return out;
}

// Extract the call-name range used as `fromRange` in incomingCalls /
// outgoingCalls aggregation. mType parser SourceLocations are not aligned
// to the call's name token for FunctionCallNode/NewNode/Super*CallNode
// (they point AFTER `)` or at the `super` keyword), so we resolve via a
// source-text scan against the document's line.
//
// For static FunctionCallNodes whose name is `ClassName::method` (the
// shape produced by parseScopeResolution), we strip the qualifier and
// scan for the method-name token.
inline std::pair<std::string, Range> callFromRange(::ast::ASTNode* call, const Document* doc) {
    if (auto* fc = dynamic_cast<nf::FunctionCallNode*>(call)) {
        std::string name = fc->getFunctionName();
        std::size_t scope = name.rfind("::");
        std::string lookup = scope == std::string::npos ? name : name.substr(scope + 2);
        return {name, nameTokenRange(doc, fc->getLocation(), lookup)};
    }
    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(call)) {
        return {mc->getMethodName(), nameTokenRange(doc, mc->getLocation(), mc->getMethodName())};
    }
    if (auto* smc = dynamic_cast<nc::SuperMethodCallNode*>(call)) {
        return {smc->getMethodName(), nameTokenRange(doc, smc->getLocation(), smc->getMethodName())};
    }
    if (auto* nn = dynamic_cast<nc::NewNode*>(call)) {
        return {nn->getClassName(), nameTokenRange(doc, nn->getLocation(), nn->getClassName())};
    }
    if (auto* tcc = dynamic_cast<nc::ThisConstructorCallNode*>(call)) {
        return {"this", nameTokenRange(doc, tcc->getLocation(), "this")};
    }
    if (auto* scc = dynamic_cast<nc::SuperConstructorCallNode*>(call)) {
        return {"super", nameTokenRange(doc, scc->getLocation(), "super")};
    }
    return {"", Range{}};
}

// Reusable recursive AST walker for call collection. Visits lambda bodies
// transparently (Q5). Caller-supplied callback runs on every call node.
class CallWalker {
public:
    using Callback = std::function<void(::ast::ASTNode*)>;
    explicit CallWalker(Callback cb) : cb_(std::move(cb)) {}
    void walk(::ast::ASTNode* n);
private:
    Callback cb_;
};

} // namespace mtype::lsp::callhier
