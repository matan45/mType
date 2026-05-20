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
        out.push_back({kKindFunction, "", fc->getFunctionName()});
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
        // Name-only: every class with a method of that name across open docs.
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
// outgoingCalls aggregation. The call node's location points at the
// name token for FunctionCallNode and MethodCallNode (per the parser);
// for NewNode/Super*CallNode we approximate by using the call's
// location with the relevant name's length.
inline std::pair<std::string, Range> callFromRange(::ast::ASTNode* call) {
    if (auto* fc = dynamic_cast<nf::FunctionCallNode*>(call)) {
        return {fc->getFunctionName(), tokenRange(fc->getLocation(), fc->getFunctionName().size())};
    }
    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(call)) {
        return {mc->getMethodName(), tokenRange(mc->getLocation(), mc->getMethodName().size())};
    }
    if (auto* smc = dynamic_cast<nc::SuperMethodCallNode*>(call)) {
        return {smc->getMethodName(), tokenRange(smc->getLocation(), smc->getMethodName().size())};
    }
    if (auto* nn = dynamic_cast<nc::NewNode*>(call)) {
        return {nn->getClassName(), tokenRange(nn->getLocation(), nn->getClassName().size())};
    }
    if (auto* tcc = dynamic_cast<nc::ThisConstructorCallNode*>(call)) {
        return {"this", tokenRange(tcc->getLocation(), 4)};
    }
    if (auto* scc = dynamic_cast<nc::SuperConstructorCallNode*>(call)) {
        return {"super", tokenRange(scc->getLocation(), 5)};
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
