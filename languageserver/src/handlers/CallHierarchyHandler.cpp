#include "CallHierarchyHandler.hpp"
#include "CallHierarchyHandlerInternal.hpp"

#include "../../../mType/ast/nodes/statements/BlockNode.hpp"
#include "../../../mType/ast/nodes/statements/IfNode.hpp"
#include "../../../mType/ast/nodes/statements/WhileNode.hpp"
#include "../../../mType/ast/nodes/statements/DoWhileNode.hpp"
#include "../../../mType/ast/nodes/statements/ForNode.hpp"
#include "../../../mType/ast/nodes/statements/ForEachNode.hpp"
#include "../../../mType/ast/nodes/statements/TryNode.hpp"
#include "../../../mType/ast/nodes/statements/CatchNode.hpp"
#include "../../../mType/ast/nodes/statements/ThrowNode.hpp"
#include "../../../mType/ast/nodes/statements/AssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/SwitchNode.hpp"
#include "../../../mType/ast/nodes/statements/CaseNode.hpp"
#include "../../../mType/ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchCaseNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchDefaultNode.hpp"
#include "../../../mType/ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../mType/ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../mType/ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../mType/ast/nodes/expressions/LambdaNode.hpp"
#include "../../../mType/ast/nodes/expressions/CastExpression.hpp"
#include "../../../mType/ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../../mType/ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../mType/ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../mType/ast/nodes/functions/ReturnNode.hpp"

namespace mtype::lsp::callhier {

void CallWalker::walk(::ast::ASTNode* n) {
    if (!n) return;
    if (auto* fc = dynamic_cast<nf::FunctionCallNode*>(n)) {
        cb_(fc);
        for (const auto& a : fc->getArguments()) walk(a.get());
        return;
    }
    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(n)) {
        cb_(mc);
        walk(mc->getObject());
        for (const auto& a : mc->getArguments()) walk(a.get());
        return;
    }
    if (auto* smc = dynamic_cast<nc::SuperMethodCallNode*>(n)) {
        cb_(smc);
        for (const auto& a : smc->getArguments()) walk(a.get());
        return;
    }
    if (auto* nn = dynamic_cast<nc::NewNode*>(n)) {
        cb_(nn);
        for (const auto& a : nn->getArguments()) walk(a.get());
        return;
    }
    if (auto* tcc = dynamic_cast<nc::ThisConstructorCallNode*>(n)) {
        cb_(tcc);
        for (const auto& a : tcc->getArguments()) walk(a.get());
        return;
    }
    if (auto* scc = dynamic_cast<nc::SuperConstructorCallNode*>(n)) {
        cb_(scc);
        for (const auto& a : scc->getArguments()) walk(a.get());
        return;
    }
    if (auto* p = dynamic_cast<ns::ProgramNode*>(n)) {
        for (const auto& s : p->getStatements()) walk(s.get()); return;
    }
    if (auto* b = dynamic_cast<ns::BlockNode*>(n)) {
        for (const auto& s : b->getStatements()) walk(s.get()); return;
    }
    if (auto* lam = dynamic_cast<ne::LambdaNode*>(n)) {
        walk(lam->getBody()); return;
    }
    // Declarations: descend into their bodies so a whole-program walk (from
    // prepare's call-site search) reaches calls inside methods/functions/
    // constructors. Incoming/outgoing pre-extract the body and never see
    // these nodes directly.
    if (auto* fn = dynamic_cast<nf::FunctionNode*>(n)) {
        walk(fn->getBodyPtr()); return;
    }
    if (auto* cls = dynamic_cast<nc::ClassNode*>(n)) {
        for (const auto& m : cls->getMethods()) walk(m.get());
        for (const auto& c : cls->getConstructors()) walk(c.get());
        return;
    }
    if (auto* mn = dynamic_cast<nc::MethodNode*>(n)) {
        walk(mn->getBodyPtr()); return;
    }
    if (auto* cn = dynamic_cast<nc::ConstructorNode*>(n)) {
        walk(cn->getBodyPtr());
        if (auto* sup = cn->getSuperInitializer()) walk(sup);
        return;
    }
    if (auto* ifn = dynamic_cast<ns::IfNode*>(n)) {
        walk(ifn->getCondition());
        walk(ifn->getThenStatement());
        walk(ifn->getElseStatement());
        return;
    }
    if (auto* w = dynamic_cast<ns::WhileNode*>(n)) {
        walk(w->getCondition()); walk(w->getBody()); return;
    }
    if (auto* dw = dynamic_cast<ns::DoWhileNode*>(n)) {
        walk(dw->getCondition()); walk(dw->getBody()); return;
    }
    if (auto* fr = dynamic_cast<ns::ForNode*>(n)) {
        walk(fr->getInitialization()); walk(fr->getCondition());
        walk(fr->getUpdate()); walk(fr->getBody());
        return;
    }
    if (auto* fe = dynamic_cast<ns::ForEachNode*>(n)) {
        walk(fe->getCollection()); walk(fe->getBody()); return;
    }
    if (auto* sw = dynamic_cast<ns::SwitchNode*>(n)) {
        walk(sw->getExpression());
        for (const auto& c : sw->getCases()) walk(c.get());
        return;
    }
    if (auto* cs = dynamic_cast<ns::CaseNode*>(n)) {
        walk(cs->getValue());
        for (const auto& s : cs->getStatements()) walk(s.get());
        return;
    }
    if (auto* dc = dynamic_cast<ns::DefaultCaseNode*>(n)) {
        for (const auto& s : dc->getStatements()) walk(s.get());
        return;
    }
    if (auto* mn = dynamic_cast<ns::MatchNode*>(n)) {
        walk(mn->getExpression());
        for (const auto& c : mn->getCases()) walk(c.get());
        return;
    }
    if (auto* mc = dynamic_cast<ns::MatchCaseNode*>(n)) {
        walk(mc->getValueExpression()); walk(mc->getBody()); return;
    }
    if (auto* md = dynamic_cast<ns::MatchDefaultNode*>(n)) {
        walk(md->getBody()); return;
    }
    if (auto* t = dynamic_cast<ns::TryNode*>(n)) {
        walk(t->getTryBlock());
        for (const auto& c : t->getCatchBlocks()) walk(c.get());
        walk(t->getFinallyBlock());
        return;
    }
    if (auto* ch = dynamic_cast<ns::CatchNode*>(n)) {
        walk(ch->getBody()); return;
    }
    if (auto* th = dynamic_cast<ns::ThrowNode*>(n)) {
        walk(th->getException()); return;
    }
    if (auto* r = dynamic_cast<nf::ReturnNode*>(n)) {
        walk(r->getReturnValue()); return;
    }
    if (auto* a = dynamic_cast<ns::AssignmentNode*>(n)) {
        walk(a->getValue()); return;
    }
    if (auto* ma = dynamic_cast<ns::MemberAssignmentNode*>(n)) {
        walk(ma->getObject()); walk(ma->getValue()); return;
    }
    if (auto* ia = dynamic_cast<ns::IndexAssignmentNode*>(n)) {
        walk(ia->getObject()); walk(ia->getIndex()); walk(ia->getValue()); return;
    }
    if (auto* be = dynamic_cast<ne::BinaryExpNode*>(n)) {
        walk(be->getLeft()); walk(be->getRight()); return;
    }
    if (auto* te = dynamic_cast<ne::TernaryExpNode*>(n)) {
        walk(te->getCondition()); walk(te->getTrueExpression()); walk(te->getFalseExpression()); return;
    }
    if (auto* ue = dynamic_cast<ne::UnaryExpNode*>(n)) {
        walk(ue->getOperand()); return;
    }
    if (auto* idx = dynamic_cast<ne::IndexAccessNode*>(n)) {
        walk(idx->getCollection()); walk(idx->getIndex()); return;
    }
    if (auto* al = dynamic_cast<ne::ArrayLiteralNode*>(n)) {
        for (const auto& e : al->getElements()) walk(e.get()); return;
    }
    if (auto* ac = dynamic_cast<ne::ArrayCreationNode*>(n)) {
        for (const auto& d : ac->getSizeExpressions()) walk(d.get()); return;
    }
    if (auto* mac = dynamic_cast<nc::MemberAccessNode*>(n)) {
        walk(mac->getObject()); return;
    }
    if (auto* ce = dynamic_cast<ne::CastExpression*>(n)) {
        walk(ce->getExpression()); return;
    }
    if (auto* io = dynamic_cast<ne::InstanceOfExpression*>(n)) {
        walk(io->getExpression()); return;
    }
    if (auto* aw = dynamic_cast<ne::AwaitExpression*>(n)) {
        walk(aw->getExpressionPtr()); return;
    }
}

} // namespace mtype::lsp::callhier

namespace mtype::lsp {

using namespace callhier;

CallHierarchyHandler::CallHierarchyHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr)
    , workspaceIndex_(std::move(workspaceIndex)) {}

namespace {

// Resolve a cursor sitting on a callable declaration's name token. Returns
// at most one item: the early returns short-circuit on the first match,
// since a declaration name is unambiguous at a single cursor position.
void appendDeclarationsAtCursor(const std::string& uri,
                                const Document* doc,
                                const std::string& word,
                                const Position& pos,
                                std::vector<CallHierarchyItem>& out) {
    if (!doc) return;
    auto* prog = getProgram(doc);
    if (!prog) return;

    auto funcs = collectFunctionsByName(doc, word);
    for (auto* fn : funcs) {
        if (rangeContains(nameTokenRange(doc, fn->getLocation(), fn->getName()), pos)) {
            out.push_back(itemForFunction(uri, fn, static_cast<int>(funcs.size())));
            return;
        }
    }

    for (auto* cls : collectClasses(doc)) {
        auto methods = collectMethodsByName(cls, word);
        for (auto* m : methods) {
            if (rangeContains(nameTokenRange(doc, m->getLocation(), m->getName()), pos)) {
                out.push_back(itemForMethod(uri, cls->getClassName(), m, static_cast<int>(methods.size())));
                return;
            }
        }
        if (word == "constructor") {
            auto ctors = collectConstructors(cls);
            for (auto* c : ctors) {
                Range r = tokenRange(c->getLocation(), kConstructorKeywordLen);
                if (rangeContains(r, pos)) {
                    out.push_back(itemForConstructor(uri, cls->getClassName(), c, static_cast<int>(ctors.size())));
                    return;
                }
            }
        }
        // Class header: cursor on `Foo` in `class Foo {` → constructor item if
        // an explicit constructor exists. ClassNode::getLocation() points at
        // the class-name token directly, so we use it without an offset.
        if (cls->getClassName() == word) {
            auto ctors = collectConstructors(cls);
            if (!ctors.empty() && rangeContains(
                    nameTokenRange(doc, cls->getLocation(), cls->getClassName()), pos)) {
                out.push_back(itemForConstructor(uri, cls->getClassName(), ctors.front(),
                                                 static_cast<int>(ctors.size())));
                return;
            }
        }
    }
}

// Determine the enclosing class for a position (used for `this.method()`
// resolution at call sites in prepare). Best-effort: picks the last
// class header at or before the cursor's line.
//
// KNOWN LIMITATION: we don't have a class end-location, so a top-level
// function declared between two classes is misattributed to the
// preceding class. Fix requires extending ClassNode with a body-end
// position (or scanning to the matching `}`) before this check.
std::string enclosingClassAt(const Document* doc, const Position& pos) {
    if (!doc) return "";
    nc::ClassNode* picked = nullptr;
    int pickedLine = -1;
    for (auto* cls : collectClasses(doc)) {
        int line = toLspPosition(cls->getLocation()).line;
        if (line <= pos.line && line > pickedLine) {
            picked = cls;
            pickedLine = line;
        }
    }
    return picked ? picked->getClassName() : "";
}

// Find call sites at the cursor. mType call-node SourceLocations are not
// aligned to the name token (FunctionCallNode/NewNode point AFTER `)`),
// so we match by cursor word + a line scan that picks the occurrence
// containing the cursor — handles `Box b = new Box();` where `Box`
// appears twice on the same line.
void appendCallSitesAtCursor(const std::string& uri,
                             const Document* doc,
                             const Position& pos,
                             const std::string& word,
                             DocumentManager* mgr,
                             std::vector<CallHierarchyItem>& out) {
    if (!doc || word.empty()) return;
    auto* prog = getProgram(doc);
    if (!prog) return;

    std::string enclosingClass = enclosingClassAt(doc, pos);
    ::ast::ASTNode* hit = nullptr;
    CallWalker walker([&](::ast::ASTNode* call) {
        if (hit) return;
        auto [name, _r] = callFromRange(call, doc);
        if (name.empty()) return;
        std::size_t scope = name.rfind("::");
        std::string lookup = scope == std::string::npos ? name : name.substr(scope + 2);
        if (lookup != word) return;
        int start = 0;
        while (true) {
            int col = findTokenColumn(doc->content, pos.line, lookup, start);
            if (col < 0) break;
            Range r{Position{pos.line, col},
                    Position{pos.line, col + static_cast<int>(lookup.size())}};
            if (rangeContains(r, pos)) { hit = call; return; }
            start = col + 1;
        }
    });
    walker.walk(prog);
    if (!hit) return;

    auto targets = resolveCall(hit, mgr, enclosingClass);
    for (const auto& t : targets) {
        if (t.kind == kKindFunction) {
            for (const auto& docUri : mgr->getAllOpenUris()) {
                auto* d = mgr->getDocument(docUri);
                if (!d) continue;
                auto fns = collectFunctionsByName(d, t.name);
                if (!fns.empty()) {
                    out.push_back(itemForFunction(docUri, fns.front(), static_cast<int>(fns.size())));
                    break;
                }
            }
        } else if (t.kind == kKindMethod) {
            auto [cls, clsUri] = findClassEverywhere(mgr, t.className);
            if (!cls) continue;
            auto methods = collectMethodsByName(cls, t.name);
            if (methods.empty()) continue;
            out.push_back(itemForMethod(clsUri, t.className, methods.front(),
                                        static_cast<int>(methods.size())));
        } else if (t.kind == kKindConstructor) {
            auto [cls, clsUri] = findClassEverywhere(mgr, t.className);
            if (!cls) continue;
            auto ctors = collectConstructors(cls);
            if (ctors.empty()) continue;
            out.push_back(itemForConstructor(clsUri, t.className, ctors.front(),
                                             static_cast<int>(ctors.size())));
        }
    }
}

} // anonymous namespace

std::vector<CallHierarchyItem> CallHierarchyHandler::handlePrepare(const std::string& uri,
                                                                    const Position& position) {
    std::vector<CallHierarchyItem> out;
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return out;
    std::string word = documentManager_->getWordAtPosition(uri, position.line, position.character);

    // Declaration site first; cursor on a declaration's name returns the
    // declaration's own item, never a call-target picker.
    if (!word.empty()) {
        appendDeclarationsAtCursor(uri, doc, word, position, out);
        if (!out.empty()) return out;
    }

    // Otherwise, resolve as a call site.
    appendCallSitesAtCursor(uri, doc, position, word, documentManager_, out);

    // De-duplicate by (kind, className, name) — Q8 collapses overloads,
    // and a name-only call may have produced overlapping candidates.
    std::vector<CallHierarchyItem> deduped;
    std::unordered_set<std::string> seen;
    for (auto& it : out) {
        auto id = extractId(it);
        std::string key = groupKey(id.kind, id.className, id.name);
        if (seen.insert(key).second) deduped.push_back(std::move(it));
    }
    return deduped;
}

} // namespace mtype::lsp
