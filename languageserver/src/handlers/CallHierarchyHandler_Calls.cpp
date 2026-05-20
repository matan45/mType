#include "CallHierarchyHandler.hpp"
#include "CallHierarchyHandlerInternal.hpp"

namespace mtype::lsp {

using namespace callhier;

namespace {

// Resolve a target identity (kind/className/name) to one or more bodies
// to walk for outgoing calls. Overloads collapse into one hierarchy item
// (Q8), but we still walk every overload's body when collecting calls so
// that the aggregate fromRanges spans all overloads.
struct BodyEntry {
    std::string uri;
    std::string enclosingClass; // empty for top-level functions
    ::ast::ASTNode* body;
};
std::vector<BodyEntry> bodiesForItem(const ItemId& id, DocumentManager* mgr) {
    std::vector<BodyEntry> out;
    if (!mgr) return out;
    if (id.kind == kKindFunction) {
        for (const auto& uri : mgr->getAllOpenUris()) {
            auto* doc = mgr->getDocument(uri);
            if (!doc) continue;
            for (auto* fn : collectFunctionsByName(doc, id.name)) {
                if (auto* b = fn->getBodyPtr()) out.push_back({uri, "", b});
            }
        }
    } else if (id.kind == kKindMethod) {
        auto [cls, uri] = findClassEverywhere(mgr, id.className);
        if (cls) {
            for (auto* m : collectMethodsByName(cls, id.name)) {
                if (auto* b = m->getBodyPtr()) out.push_back({uri, id.className, b});
            }
        }
    } else if (id.kind == kKindConstructor) {
        auto [cls, uri] = findClassEverywhere(mgr, id.className);
        if (cls) {
            for (auto* c : collectConstructors(cls)) {
                if (auto* b = c->getBodyPtr()) out.push_back({uri, id.className, b});
            }
        }
    }
    return out;
}

CallHierarchyItem itemFromTarget(const ResolvedTarget& t, DocumentManager* mgr) {
    if (t.kind == kKindFunction) {
        for (const auto& uri : mgr->getAllOpenUris()) {
            auto* doc = mgr->getDocument(uri);
            if (!doc) continue;
            auto fns = collectFunctionsByName(doc, t.name);
            if (!fns.empty()) {
                return itemForFunction(uri, fns.front(), static_cast<int>(fns.size()));
            }
        }
    } else if (t.kind == kKindMethod) {
        auto [cls, uri] = findClassEverywhere(mgr, t.className);
        if (cls) {
            auto ms = collectMethodsByName(cls, t.name);
            if (!ms.empty()) {
                return itemForMethod(uri, t.className, ms.front(), static_cast<int>(ms.size()));
            }
        }
    } else if (t.kind == kKindConstructor) {
        auto [cls, uri] = findClassEverywhere(mgr, t.className);
        if (cls) {
            auto cs = collectConstructors(cls);
            if (!cs.empty()) {
                return itemForConstructor(uri, t.className, cs.front(), static_cast<int>(cs.size()));
            }
        }
    }
    CallHierarchyItem stub;
    stub.name = t.name.empty() ? t.className : t.name;
    stub.kind = SymbolKind::Function;
    return stub;
}

bool resolvedTargetMatchesItem(const ResolvedTarget& t, const ItemId& itemId) {
    if (t.kind != itemId.kind) return false;
    if (t.className != itemId.className) return false;
    // For constructors, name is "" — comparing on className alone suffices.
    return t.name == itemId.name;
}

struct CallerKey {
    std::string kind, className, name;
    bool operator==(const CallerKey& o) const {
        return kind == o.kind && className == o.className && name == o.name;
    }
};
struct CallerKeyHash {
    std::size_t operator()(const CallerKey& k) const noexcept {
        return std::hash<std::string>{}(k.kind + "\x1f" + k.className + "\x1f" + k.name);
    }
};

// Per-caller accumulator: an item (built once) + ranges from every call.
struct CallerAcc {
    CallHierarchyItem item;
    std::vector<Range> ranges;
};

// Walk all callables in every open document, looking for calls that
// resolve to the target item. Tracks the enclosing real callable on a
// stack — lambdas don't push (Q5), so calls inside lambdas attribute to
// the enclosing function/method/constructor. Top-level call sites have
// no enclosing real callable and are dropped (Q7).
std::vector<CallHierarchyIncomingCall> collectIncoming(const ItemId& itemId,
                                                        DocumentManager* mgr) {
    std::unordered_map<CallerKey, CallerAcc, CallerKeyHash> byCaller;

    auto recordCall = [&](const CallerKey& key, const CallHierarchyItem& callerItem,
                          const Range& r) {
        auto it = byCaller.find(key);
        if (it == byCaller.end()) {
            CallerAcc acc;
            acc.item = callerItem;
            acc.ranges.push_back(r);
            byCaller.emplace(key, std::move(acc));
        } else {
            it->second.ranges.push_back(r);
        }
    };

    for (const auto& uri : mgr->getAllOpenUris()) {
        auto* doc = mgr->getDocument(uri);
        if (!doc) continue;
        auto* prog = getProgram(doc);
        if (!prog) continue;

        // Top-level functions are callers — walk their bodies attributed
        // to (function, name).
        for (const auto& stmt : prog->getStatements()) {
            if (auto* fn = dynamic_cast<nf::FunctionNode*>(stmt.get())) {
                auto* body = fn->getBodyPtr();
                if (!body) continue;
                CallerKey key{kKindFunction, "", fn->getName()};
                CallHierarchyItem callerItem;
                bool itemBuilt = false;
                CallWalker walker([&](::ast::ASTNode* call) {
                    for (const auto& t : resolveCall(call, mgr, "")) {
                        if (!resolvedTargetMatchesItem(t, itemId)) continue;
                        if (!itemBuilt) {
                            auto funcs = collectFunctionsByName(doc, fn->getName());
                            callerItem = itemForFunction(uri, fn,
                                static_cast<int>(funcs.size() ? funcs.size() : 1));
                            itemBuilt = true;
                        }
                        auto [nm, r] = callFromRange(call);
                        recordCall(key, callerItem, r);
                    }
                });
                walker.walk(body);
            } else if (auto* cls = dynamic_cast<nc::ClassNode*>(stmt.get())) {
                const std::string& className = cls->getClassName();
                // Methods.
                for (const auto& m : cls->getMethods()) {
                    auto* mn = dynamic_cast<nc::MethodNode*>(m.get());
                    if (!mn) continue;
                    auto* body = mn->getBodyPtr();
                    if (!body) continue;
                    CallerKey key{kKindMethod, className, mn->getName()};
                    CallHierarchyItem callerItem;
                    bool itemBuilt = false;
                    CallWalker walker([&](::ast::ASTNode* call) {
                        for (const auto& t : resolveCall(call, mgr, className)) {
                            if (!resolvedTargetMatchesItem(t, itemId)) continue;
                            if (!itemBuilt) {
                                auto ms = collectMethodsByName(cls, mn->getName());
                                callerItem = itemForMethod(uri, className, mn,
                                    static_cast<int>(ms.size() ? ms.size() : 1));
                                itemBuilt = true;
                            }
                            auto [nm, r] = callFromRange(call);
                            recordCall(key, callerItem, r);
                        }
                    });
                    walker.walk(body);
                }
                // Constructors.
                for (const auto& c : cls->getConstructors()) {
                    auto* cn = dynamic_cast<nc::ConstructorNode*>(c.get());
                    if (!cn) continue;
                    auto* body = cn->getBodyPtr();
                    if (!body) continue;
                    CallerKey key{kKindConstructor, className, ""};
                    CallHierarchyItem callerItem;
                    bool itemBuilt = false;
                    CallWalker walker([&](::ast::ASTNode* call) {
                        for (const auto& t : resolveCall(call, mgr, className)) {
                            if (!resolvedTargetMatchesItem(t, itemId)) continue;
                            if (!itemBuilt) {
                                auto cs = collectConstructors(cls);
                                callerItem = itemForConstructor(uri, className, cn,
                                    static_cast<int>(cs.size() ? cs.size() : 1));
                                itemBuilt = true;
                            }
                            auto [nm, r] = callFromRange(call);
                            recordCall(key, callerItem, r);
                        }
                    });
                    walker.walk(body);
                    // Super-constructor initializer (the `: super(...)` clause).
                    if (auto* sup = cn->getSuperInitializer()) {
                        CallWalker initWalker([&](::ast::ASTNode* call) {
                            for (const auto& t : resolveCall(call, mgr, className)) {
                                if (!resolvedTargetMatchesItem(t, itemId)) continue;
                                if (!itemBuilt) {
                                    auto cs = collectConstructors(cls);
                                    callerItem = itemForConstructor(uri, className, cn,
                                        static_cast<int>(cs.size() ? cs.size() : 1));
                                    itemBuilt = true;
                                }
                                auto [nm, r] = callFromRange(call);
                                recordCall(key, callerItem, r);
                            }
                        });
                        initWalker.walk(sup);
                    }
                }
            }
        }
    }

    std::vector<CallHierarchyIncomingCall> out;
    out.reserve(byCaller.size());
    for (auto& [k, acc] : byCaller) {
        CallHierarchyIncomingCall ic;
        ic.from = std::move(acc.item);
        ic.fromRanges = std::move(acc.ranges);
        out.push_back(std::move(ic));
    }
    return out;
}

} // anonymous namespace

std::vector<CallHierarchyIncomingCall> CallHierarchyHandler::handleIncoming(
    const CallHierarchyItem& item)
{
    ItemId id = extractId(item);
    if (id.kind.empty()) return {};
    return collectIncoming(id, documentManager_);
}

std::vector<CallHierarchyOutgoingCall> CallHierarchyHandler::handleOutgoing(
    const CallHierarchyItem& item)
{
    ItemId id = extractId(item);
    if (id.kind.empty()) return {};

    auto bodies = bodiesForItem(id, documentManager_);
    if (bodies.empty()) return {};

    struct TargetAcc {
        CallHierarchyItem item;
        std::vector<Range> ranges;
    };
    std::unordered_map<std::string, TargetAcc> byTarget;

    for (const auto& entry : bodies) {
        CallWalker walker([&](::ast::ASTNode* call) {
            auto targets = resolveCall(call, documentManager_, entry.enclosingClass);
            for (const auto& t : targets) {
                std::string key = groupKey(t.kind, t.className, t.name);
                auto [nm, r] = callFromRange(call);
                auto it = byTarget.find(key);
                if (it == byTarget.end()) {
                    TargetAcc acc;
                    acc.item = itemFromTarget(t, documentManager_);
                    // Drop synthetic stubs (target not found) — Q9 / Q1.
                    if (!acc.item.data) continue;
                    acc.ranges.push_back(r);
                    byTarget.emplace(key, std::move(acc));
                } else {
                    it->second.ranges.push_back(r);
                }
            }
        });
        walker.walk(entry.body);
    }

    std::vector<CallHierarchyOutgoingCall> out;
    out.reserve(byTarget.size());
    for (auto& [k, acc] : byTarget) {
        CallHierarchyOutgoingCall oc;
        oc.to = std::move(acc.item);
        oc.fromRanges = std::move(acc.ranges);
        out.push_back(std::move(oc));
    }
    return out;
}

} // namespace mtype::lsp
