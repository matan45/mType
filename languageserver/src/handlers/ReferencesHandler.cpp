#include "ReferencesHandler.hpp"
#include "CallHierarchyHandlerInternal.hpp"
#include "../utils/UriUtils.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace mtype::lsp {

using namespace callhier;

namespace {

// A symbol we're looking for. The (kind, className, name) triple is the
// identity used to compare against the call-site resolutions produced by
// resolveCall, so that a search for `Foo.bar` does not match `Bar.bar`.
struct TargetSymbol {
    std::string kind;       // "function", "method-instance", "method-static",
                            // "class", "constructor"
    std::string className;  // owning class; empty for free functions / classes
    std::string name;       // symbol name; empty for constructors (className is identity)
};

std::string targetKey(const TargetSymbol& t) {
    return t.kind + "\x1f" + t.className + "\x1f" + t.name;
}

bool isWordBoundary(char c) {
    return !(std::isalnum(static_cast<unsigned char>(c)) || c == '_');
}

// Find the class containing `pos`. Mirrors the helper in CallHierarchyHandler;
// inherits the same known limitation (no end-of-class location, so a top-level
// function declared between two classes is misattributed to the preceding one).
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

// Walk every class currently parsed in any open document and add a target
// for every subclass of `baseClass` that declares `methodName`. mType
// instance methods are virtual by default, so a reference search on
// Base.foo should also surface Derived.foo's declaration and any calls
// dispatched through a Base reference that may land in Derived.foo.
void addOverridesForInstanceMethod(DocumentManager* mgr,
                                    const std::string& baseClass,
                                    const std::string& methodName,
                                    std::vector<TargetSymbol>& targets) {
    if (!mgr) return;
    for (const auto& docUri : mgr->getAllOpenUris()) {
        auto* doc = mgr->getDocument(docUri);
        if (!doc) continue;
        for (auto* subCls : collectClasses(doc)) {
            if (subCls->getClassName() == baseClass) continue;
            // Walk the parent chain of subCls; if it reaches baseClass,
            // and subCls declares the method, it's an override candidate.
            std::unordered_set<std::string> seen;
            std::string cur = subCls->getClassName();
            bool inherits = false;
            while (!cur.empty() && seen.insert(cur).second) {
                auto [c, _] = findClassEverywhere(mgr, cur);
                if (!c || !c->hasParentClass()) break;
                if (c->getParentClassName() == baseClass) { inherits = true; break; }
                cur = c->getParentClassName();
            }
            if (inherits && !collectMethodsByName(subCls, methodName).empty()) {
                targets.push_back({"method-instance", subCls->getClassName(), methodName});
            }
        }
    }
}

bool isStaticCallNode(::ast::ASTNode* call) {
    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(call)) {
        return mc->getIsStaticCall();
    }
    if (auto* fc = dynamic_cast<nf::FunctionCallNode*>(call)) {
        return fc->getFunctionName().find("::") != std::string::npos;
    }
    return false;
}

// Classify the symbol at the cursor and build the set of targets to search
// for. Returns empty when nothing recognizable is under the cursor.
std::vector<TargetSymbol> buildTargetSet(const std::string& uri,
                                          const Document* doc,
                                          const std::string& word,
                                          const Position& pos,
                                          DocumentManager* mgr) {
    std::vector<TargetSymbol> targets;
    if (!doc || word.empty()) return targets;

    // Cursor on a free-function declaration name token.
    for (auto* fn : collectFunctionsByName(doc, word)) {
        if (rangeContains(nameTokenRange(doc, fn->getLocation(), fn->getName()), pos)) {
            targets.push_back({"function", "", word});
            return targets;
        }
    }

    // Cursor on a method declaration or class header.
    for (auto* cls : collectClasses(doc)) {
        for (auto* m : collectMethodsByName(cls, word)) {
            if (rangeContains(nameTokenRange(doc, m->getLocation(), m->getName()), pos)) {
                if (m->getIsStatic()) {
                    // Static methods are hidden, not overridden — no chain walk.
                    targets.push_back({"method-static", cls->getClassName(), word});
                } else {
                    targets.push_back({"method-instance", cls->getClassName(), word});
                    addOverridesForInstanceMethod(mgr, cls->getClassName(), word, targets);
                }
                return targets;
            }
        }
        if (cls->getClassName() == word &&
            rangeContains(nameTokenRange(doc, cls->getLocation(), cls->getClassName()), pos)) {
            targets.push_back({"class", "", word});
            targets.push_back({"constructor", word, ""});
            return targets;
        }
    }

    // Cursor on a call site — let resolveCall figure out the target identity.
    auto* prog = getProgram(doc);
    if (prog) {
        std::string enclosing = enclosingClassAt(doc, pos);
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

        if (hit) {
            bool isStatic = isStaticCallNode(hit);
            for (const auto& t : resolveCall(hit, mgr, enclosing)) {
                if (t.kind == kKindFunction) {
                    targets.push_back({"function", "", t.name});
                } else if (t.kind == kKindMethod) {
                    if (isStatic) {
                        targets.push_back({"method-static", t.className, t.name});
                    } else {
                        targets.push_back({"method-instance", t.className, t.name});
                        addOverridesForInstanceMethod(mgr, t.className, t.name, targets);
                    }
                } else if (t.kind == kKindConstructor) {
                    targets.push_back({"constructor", t.className, ""});
                    targets.push_back({"class", "", t.className});
                }
            }
            if (!targets.empty()) return targets;
        }
    }

    // Last resort: word is the name of a class somewhere in this document
    // (e.g., cursor on the `Foo` of `Foo myVar;` — a type annotation, which
    // is not an AST call). Fall through to a class-target textual scan.
    for (const auto& sym : mgr->getDocumentSymbols(uri)) {
        if (sym.name == word && sym.kind == "class") {
            targets.push_back({"class", "", word});
            targets.push_back({"constructor", word, ""});
            return targets;
        }
    }

    return targets;
}

// Convert a resolved call into the set of target keys that would match it.
//
// For instance method calls on a bare variable receiver, this bypasses
// resolveCall's name-only fallback (which enumerates every class that
// declares a method with the same name) and uses
// DocumentManager::getVariableType to pin the call to one class. Without
// this, a search for Foo.value would match b.value() whenever Bar also
// declares value(), because both classes show up in the fallback.
std::vector<std::string> keysForCall(::ast::ASTNode* call,
                                       DocumentManager* mgr,
                                       const std::string& docUri,
                                       const std::string& encClass) {
    std::vector<std::string> out;

    if (auto* mc = dynamic_cast<nc::MethodCallNode*>(call); mc && !mc->getIsStaticCall()) {
        if (auto* vn = dynamic_cast<ne::VariableNode*>(mc->getObject())) {
            const std::string& recv = vn->getName();
            if (recv == "this" && !encClass.empty()) {
                out.push_back(targetKey({"method-instance", encClass, mc->getMethodName()}));
                return out;
            }
            int line = toLspPosition(mc->getLocation()).line;
            std::string inferred = mgr ? mgr->getVariableType(docUri, recv, line) : "";
            if (!inferred.empty()) {
                out.push_back(targetKey({"method-instance", inferred, mc->getMethodName()}));
                return out;
            }
        }
    }

    bool isStatic = isStaticCallNode(call);
    for (const auto& t : resolveCall(call, mgr, encClass)) {
        if (t.kind == kKindFunction) {
            out.push_back(targetKey({"function", "", t.name}));
        } else if (t.kind == kKindMethod) {
            out.push_back(targetKey({isStatic ? "method-static" : "method-instance",
                                     t.className, t.name}));
        } else if (t.kind == kKindConstructor) {
            out.push_back(targetKey({"constructor", t.className, ""}));
        }
    }
    return out;
}

// Walk a document, calling resolveCall on every call node with the right
// enclosing class. Emits a Location for each call whose resolved target
// is in `targetKeys`. CallWalker descends into method/function/constructor
// bodies on its own, but it doesn't track enclosing class — we visit the
// program's top-level statements ourselves so each method body is walked
// with its owning class name in scope.
void scanDocumentForCalls(const std::string& docUri,
                          const Document* doc,
                          DocumentManager* mgr,
                          const std::unordered_set<std::string>& targetKeys,
                          std::vector<Location>& results) {
    auto* prog = getProgram(doc);
    if (!prog) return;

    auto walkBody = [&](::ast::ASTNode* body, const std::string& encClass) {
        if (!body) return;
        CallWalker w([&](::ast::ASTNode* call) {
            for (const auto& k : keysForCall(call, mgr, docUri, encClass)) {
                if (targetKeys.count(k)) {
                    auto [_n, r] = callFromRange(call, doc);
                    Location loc;
                    loc.uri = docUri;
                    loc.range = r;
                    results.push_back(loc);
                    return;
                }
            }
        });
        w.walk(body);
    };

    for (const auto& s : prog->getStatements()) {
        ::ast::ASTNode* node = s.get();
        if (auto* cls = dynamic_cast<nc::ClassNode*>(node)) {
            for (const auto& m : cls->getMethods()) {
                if (auto* mn = dynamic_cast<nc::MethodNode*>(m.get())) {
                    walkBody(mn->getBodyPtr(), cls->getClassName());
                }
            }
            for (const auto& c : cls->getConstructors()) {
                if (auto* cn = dynamic_cast<nc::ConstructorNode*>(c.get())) {
                    walkBody(cn->getBodyPtr(), cls->getClassName());
                    if (auto* sup = cn->getSuperInitializer()) {
                        walkBody(sup, cls->getClassName());
                    }
                }
            }
            continue;
        }
        if (auto* fn = dynamic_cast<nf::FunctionNode*>(node)) {
            walkBody(fn->getBodyPtr(), "");
            continue;
        }
        // Free-floating top-level statement (assignment, expression, etc.):
        // walk it directly with no enclosing class.
        walkBody(node, "");
    }
}

// Textual whole-word scan used for class-target references (type annotations,
// extends, implements, static-access prefix `Foo::`). Class declaration lines
// are skipped unless includeDeclaration is true.
std::vector<Location> scanClassRefs(const std::string& className,
                                      const std::string& uri,
                                      const std::string& content,
                                      bool includeDeclaration) {
    std::vector<Location> refs;
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    std::regex declRegex(R"(^\s*(?:abstract\s+)?(?:class|interface)\s+)" + className + R"(\b)");

    while (std::getline(stream, line)) {
        if (!includeDeclaration && std::regex_search(line, declRegex)) {
            lineNum++;
            continue;
        }
        std::size_t startIdx = 0;
        while (true) {
            std::size_t pos = line.find(className, startIdx);
            if (pos == std::string::npos) break;
            char prevChar = (pos > 0) ? line[pos - 1] : ' ';
            char nextChar = (pos + className.length() < line.length())
                            ? line[pos + className.length()] : ' ';
            if (isWordBoundary(prevChar) && isWordBoundary(nextChar)) {
                Location loc;
                loc.uri = uri;
                loc.range.start.line = lineNum;
                loc.range.start.character = static_cast<int>(pos);
                loc.range.end.line = lineNum;
                loc.range.end.character = static_cast<int>(pos + className.length());
                refs.push_back(loc);
            }
            startIdx = pos + 1;
        }
        lineNum++;
    }
    return refs;
}

// Textual whole-word scan for variables. Variable scope inference is
// deferred to rename; for references we accept the same line-scoped
// approximation the original handler used.
std::vector<Location> scanVariableRefs(const std::string& varName,
                                         const std::string& uri,
                                         const std::string& content,
                                         bool includeDeclaration) {
    std::vector<Location> refs;
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    std::regex declRegex(R"(\b\w+\s+)" + varName + R"(\s*[;=])");

    while (std::getline(stream, line)) {
        bool isDeclarationLine = !includeDeclaration && std::regex_search(line, declRegex);

        std::size_t startIdx = 0;
        while (true) {
            std::size_t pos = line.find(varName, startIdx);
            if (pos == std::string::npos) break;
            char prevChar = (pos > 0) ? line[pos - 1] : ' ';
            char nextChar = (pos + varName.length() < line.length())
                            ? line[pos + varName.length()] : ' ';
            if (isWordBoundary(prevChar) && isWordBoundary(nextChar)) {
                if (isDeclarationLine) {
                    std::string beforeMatch = line.substr(0, pos);
                    std::string afterMatch = line.substr(pos + varName.length());
                    std::regex localDeclCheck(R"(\w+\s+$)");
                    std::regex afterDeclCheck(R"(^\s*[;=])");
                    if (std::regex_search(beforeMatch, localDeclCheck) &&
                        std::regex_search(afterMatch, afterDeclCheck)) {
                        startIdx = pos + 1;
                        continue;
                    }
                }
                Location loc;
                loc.uri = uri;
                loc.range.start.line = lineNum;
                loc.range.start.character = static_cast<int>(pos);
                loc.range.end.line = lineNum;
                loc.range.end.character = static_cast<int>(pos + varName.length());
                refs.push_back(loc);
            }
            startIdx = pos + 1;
        }
        lineNum++;
    }
    return refs;
}

// Ensure every workspace-indexed `.mt` file is parsed and reachable via
// DocumentManager, by reading from disk and opening any URI that isn't
// already an in-memory document. This is what makes Find References
// workspace-wide rather than open-files-only; without it the AST walk
// below sees only the files the user has opened in editor tabs.
//
// Files opened here stay in DocumentManager. A later didOpen from the
// editor will replace the disk-loaded entry with the live editor buffer,
// so we never serve stale content to other handlers. Cost: one-time
// parse per workspace file on first reference query — acceptable for a
// human-triggered command, and amortised across subsequent calls.
void ensureWorkspaceDocsParsed(DocumentManager* mgr,
                                analysis::WorkspaceSymbolIndex* idx) {
    if (!mgr || !idx) return;
    idx->waitForReady(std::chrono::milliseconds(200));
    for (const auto& uri : idx->getAllIndexedFiles()) {
        if (mgr->getDocument(uri)) continue;
        std::string path = UriUtils::uriToFilePath(uri);
        std::ifstream stream(path, std::ios::binary);
        if (!stream) continue;
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        mgr->openDocument(uri, buffer.str(), 1);
    }
}

// Append the name-token range of every declaration that matches a target
// in the set. Used when includeDeclaration is true so the user sees the
// declaration alongside the call sites.
void appendDeclarations(const std::vector<TargetSymbol>& targets,
                          DocumentManager* mgr,
                          std::vector<Location>& results) {
    for (const auto& docUri : mgr->getAllOpenUris()) {
        auto* doc = mgr->getDocument(docUri);
        if (!doc) continue;
        for (const auto& t : targets) {
            if (t.kind == "function") {
                for (auto* fn : collectFunctionsByName(doc, t.name)) {
                    Range r = nameTokenRange(doc, fn->getLocation(), fn->getName());
                    Location loc; loc.uri = docUri; loc.range = r;
                    results.push_back(loc);
                }
            } else if (t.kind == "method-instance" || t.kind == "method-static") {
                bool wantStatic = (t.kind == "method-static");
                for (auto* cls : collectClasses(doc)) {
                    if (cls->getClassName() != t.className) continue;
                    for (auto* m : collectMethodsByName(cls, t.name)) {
                        if (m->getIsStatic() != wantStatic) continue;
                        Range r = nameTokenRange(doc, m->getLocation(), m->getName());
                        Location loc; loc.uri = docUri; loc.range = r;
                        results.push_back(loc);
                    }
                }
            }
        }
    }
}

} // anonymous namespace

ReferencesHandler::ReferencesHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr)
    , workspaceIndex_(std::move(workspaceIndex))
{
}

std::vector<Location> ReferencesHandler::handleReferences(
    const std::string& uri,
    const Position& position,
    bool includeDeclaration)
{
    std::vector<Location> results;
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return results;

    std::string word = documentManager_->getWordAtPosition(uri, position.line, position.character);
    if (word.empty()) return results;

    // Pull every workspace-indexed file into DocumentManager so the scan
    // below covers files the user hasn't opened in editor tabs.
    ensureWorkspaceDocsParsed(documentManager_, workspaceIndex_.get());

    auto targets = buildTargetSet(uri, doc, word, position, documentManager_);

    // Nothing recognizable under the cursor — preserve legacy variable
    // behavior by doing a single-document textual whole-word scan. This is
    // what users get for local variables and parameters; rename is the
    // correct tool for scoped variable refactors.
    if (targets.empty()) {
        return scanVariableRefs(word, uri, doc->content, includeDeclaration);
    }

    std::unordered_set<std::string> targetKeys;
    for (const auto& t : targets) targetKeys.insert(targetKey(t));

    bool hasClassTarget = false;
    std::string classTargetName;
    for (const auto& t : targets) {
        if (t.kind == "class") {
            hasClassTarget = true;
            classTargetName = t.name;
            break;
        }
    }

    // Walk every open document. AST-based call scan handles function /
    // method / constructor targets. Class targets use the textual scan
    // because type annotations / extends / implements are not call sites.
    for (const auto& docUri : documentManager_->getAllOpenUris()) {
        auto* otherDoc = documentManager_->getDocument(docUri);
        if (!otherDoc) continue;

        scanDocumentForCalls(docUri, otherDoc, documentManager_, targetKeys, results);

        if (hasClassTarget) {
            auto classRefs = scanClassRefs(classTargetName, docUri, otherDoc->content, includeDeclaration);
            results.insert(results.end(), classRefs.begin(), classRefs.end());
        }
    }

    if (includeDeclaration) {
        appendDeclarations(targets, documentManager_, results);
    }

    // Class targets emit a Location for the class-name token via the textual
    // scan, and the AST scan emits another Location for the same token in
    // `new Foo()`. Collapse identical (uri, range) entries.
    std::sort(results.begin(), results.end(), [](const Location& a, const Location& b) {
        if (a.uri != b.uri) return a.uri < b.uri;
        if (a.range.start.line != b.range.start.line) return a.range.start.line < b.range.start.line;
        if (a.range.start.character != b.range.start.character) return a.range.start.character < b.range.start.character;
        if (a.range.end.line != b.range.end.line) return a.range.end.line < b.range.end.line;
        return a.range.end.character < b.range.end.character;
    });
    results.erase(std::unique(results.begin(), results.end(),
        [](const Location& a, const Location& b) {
            return a.uri == b.uri
                && a.range.start.line == b.range.start.line
                && a.range.start.character == b.range.start.character
                && a.range.end.line == b.range.end.line
                && a.range.end.character == b.range.end.character;
        }), results.end());

    return results;
}

} // namespace mtype::lsp
