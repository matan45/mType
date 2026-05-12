#include "RenameHandler.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../mType/token/TokenType.hpp"
#include "../utils/LSPUtils.hpp"

namespace mtype::lsp {

RenameHandler::RenameHandler(DocumentManager* docMgr)
    : documentManager_(docMgr)
{}

bool RenameHandler::isValidIdentifier(const std::string& s) {
    if (s.empty()) return false;
    unsigned char first = static_cast<unsigned char>(s[0]);
    if (first > 127) return false;
    if (!(std::isalpha(first) || first == '_')) return false;
    for (size_t i = 1; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c > 127) return false;
        if (!(std::isalnum(c) || c == '_')) return false;
    }
    return true;
}

bool RenameHandler::isReservedKeyword(const std::string& s) {
    // Mirrors mType/lexer/Lexer.cpp's keywords map. Duplicating ~30 strings
    // is preferable to pulling the lexer header into this translation unit
    // for a single check.
    static const std::unordered_set<std::string> kKeywords = {
        "function", "return", "if", "else", "while", "for", "do",
        "break", "continue", "switch", "case", "default",
        "int", "float", "bool", "string", "void", "object",
        "true", "false", "null",
        "class", "interface", "extends", "implements", "new",
        "static", "private", "public", "protected", "abstract",
        "final", "value", "constructor",
        "import", "from", "lib",
        "async", "await", "try", "catch", "throw", "finally",
        "isClassOf", "super", "this", "match", "annotation"
    };
    return kKeywords.count(s) > 0;
}

bool RenameHandler::isAsciiLine(const std::string& line) {
    for (unsigned char c : line) {
        if (c > 127) return false;
    }
    return true;
}

bool RenameHandler::isUriUnderLib(const std::string& uri) {
    std::string normalized = uri;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized.find("/lib/") != std::string::npos;
}

Range RenameHandler::tokenRange(const token::Token& tok) {
    Range r{};
    r.start.line = tok.location.getLine() - 1;
    r.start.character = tok.location.getColumn() - 1;
    r.end.line = r.start.line;
    r.end.character = r.start.character + static_cast<int>(tok.length);
    return r;
}

namespace {
using utils::tokenText;
} // namespace

const token::Token* RenameHandler::findIdentifierTokenAt(
    const std::vector<token::Token>& tokens,
    int line,
    int character) const
{
    for (const auto& tok : tokens) {
        if (tok.type != token::TokenType::IDENTIFIER) continue;
        int tokLine = tok.location.getLine() - 1;
        int tokCol = tok.location.getColumn() - 1;
        if (tokLine != line) continue;
        int tokEnd = tokCol + static_cast<int>(tok.length);
        if (character >= tokCol && character <= tokEnd) {
            return &tok;
        }
    }
    return nullptr;
}

bool RenameHandler::classifyTarget(const std::string& uri,
                                    const Position& position,
                                    TargetInfo& out,
                                    std::string& error)
{
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) {
        error = "document not open";
        return false;
    }
    if (!doc->isParsed) {
        error = "document not yet parsed";
        return false;
    }

    // Find the identifier token under the cursor. If the cursor sits on
    // a literal, keyword, operator, or whitespace, findIdentifierTokenAt
    // returns null — those are exactly the cases the AC asks us to
    // reject up front.
    const token::Token* tok = findIdentifierTokenAt(
        doc->tokens, position.line, position.character);
    if (!tok) {
        error = "cursor is not on a renameable identifier";
        return false;
    }

    // Extract the lexeme from doc->content — tok->stringValue.data()
    // is a dangling pointer after parseDocument's temporary lexer is
    // destroyed. See utils::tokenText (LSPUtils.hpp) for the rationale.
    std::string name = tokenText(doc, *tok);
    if (name.empty()) {
        error = "cursor is not on a renameable identifier";
        return false;
    }

    if (isReservedKeyword(name)) {
        error = "cannot rename a reserved keyword";
        return false;
    }

    // ASCII guard — v1 does not implement UTF-8 ↔ UTF-16 column
    // translation, so any non-ASCII byte on the cursor line means we
    // cannot produce a spec-correct LSP Range and must reject.
    {
        std::istringstream stream(doc->content);
        std::string ln;
        int n = 0;
        while (std::getline(stream, ln)) {
            if (n == position.line) {
                if (!isAsciiLine(ln)) {
                    error = "non-ASCII identifiers are not supported in v1";
                    return false;
                }
                break;
            }
            n++;
        }
    }

    out.name = name;
    out.cursorRange = tokenRange(*tok);

    // SymbolRegistrationVisitor stores positions as 0-based; tokens
    // carry 1-based locations.
    int tokLine = tok->location.getLine() - 1;

    auto applyEntry = [&](const std::string& key, const SymbolLocationInfo& info) {
        auto dotPos = key.find('.');
        if (dotPos == std::string::npos) {
            out.kind = SymbolKind::TopLevel;
        } else {
            out.kind = SymbolKind::Member;
            out.declaringClass = key.substr(0, dotPos);
        }
        out.defUri = info.uri;
    };

    // Match by (same line, key name == cursor identifier). The visitor
    // anchors method/field locations to whichever sub-token the parser
    // assigned to the declaration node — often the name itself, but
    // sometimes the `function` keyword or access modifier. Comparing
    // line-and-name is robust to that choice and is unambiguous because
    // mType cannot declare two same-named symbols on the same line.
    auto findEntryByName = [&](const Document* d, int ln, const std::string& nm) -> bool {
        if (!d) return false;
        for (const auto& [key, info] : d->symbolLocations) {
            if (info.line != ln) continue;
            auto dotPos = key.find('.');
            const std::string keyName = (dotPos == std::string::npos)
                ? key : key.substr(dotPos + 1);
            if (keyName == nm) {
                applyEntry(key, info);
                return true;
            }
        }
        return false;
    };

    // Case 1 — the cursor is on a declaration site (e.g., the user
    // pressed F2 on `class Foo`). findDefinition only resolves
    // *usage*-side cursors, so we check the current doc's
    // symbolLocations directly first.
    if (findEntryByName(doc, tokLine, name)) {
        if (isUriUnderLib(out.defUri)) {
            out.isBuiltin = true;
            error = "cannot rename built-in symbol '" + name + "' (defined in lib/)";
            return false;
        }
        return true;
    }

    // Case 2 — the cursor is on a usage. findDefinition resolves to
    // the declaration in the same or another open document; we then
    // classify by looking up the entry at that resolved position.
    auto defOpt = documentManager_->findDefinition(
        uri, position.line, position.character);
    if (defOpt) {
        if (isUriUnderLib(defOpt->uri)) {
            out.isBuiltin = true;
            error = "cannot rename built-in symbol '" + name + "' (defined in lib/)";
            return false;
        }
        const Document* defDoc = documentManager_->getDocument(defOpt->uri);
        if (findEntryByName(defDoc, defOpt->line, name)) {
            return true;
        }
        // Has a def site but no indexed entry — defensive fall-through
        // to Local below.
    }

    // Case 3 — no symbol entry anywhere. Treat as a local/parameter.
    // The localIsUnambiguous gate later enforces single-declaration
    // safety before any edits are produced.
    out.kind = SymbolKind::Local;
    out.defUri = uri;
    return true;
}

std::vector<std::string> RenameHandler::collectOverrideSet(
    const std::string& declaringClass) const
{
    std::set<std::string> set;
    if (declaringClass.empty()) return {};
    set.insert(declaringClass);
    // Every mType class implicitly extends Object, so renaming a method
    // declared in Object's transitive chain is always a "this-class"
    // rename rather than a cross-class collision.
    set.insert("Object");

    // The LSP's SymbolRegistrationVisitor does NOT call
    // ClassRegistry::registerInheritance, so the registry's
    // InheritanceTracker can't answer getInheritanceChain or
    // getChildClasses. Walk the parent links directly off
    // ClassDefinition::parentClassName instead. Union the view from
    // every open document's registry.
    auto allClasses = [&]() {
        std::unordered_map<std::string, std::string> childToParent;
        for (const auto& fileUri : documentManager_->getAllOpenUris()) {
            const Document* doc = documentManager_->getDocument(fileUri);
            if (!doc || !doc->environment) continue;
            auto reg = doc->environment->getClassRegistry();
            if (!reg) continue;
            for (const auto& name : reg->getAllItemNames()) {
                auto cls = reg->findClass(name);
                if (!cls) continue;
                const auto& parent = cls->getParentClassName();
                if (!parent.empty()) {
                    childToParent[name] = parent;
                }
            }
        }
        return childToParent;
    }();

    // Ancestors: walk up declaringClass → parent → grandparent until
    // empty or we revisit (defensive cycle guard).
    {
        std::string cur = declaringClass;
        std::unordered_set<std::string> seen;
        while (!cur.empty() && seen.insert(cur).second) {
            auto it = allClasses.find(cur);
            if (it == allClasses.end()) break;
            set.insert(it->second);
            cur = it->second;
        }
    }

    // Descendants: invert the child→parent map and DFS from
    // declaringClass.
    std::unordered_map<std::string, std::vector<std::string>> parentToChildren;
    for (const auto& [child, parent] : allClasses) {
        parentToChildren[parent].push_back(child);
    }
    {
        std::vector<std::string> stack = { declaringClass };
        while (!stack.empty()) {
            auto cur = std::move(stack.back());
            stack.pop_back();
            auto it = parentToChildren.find(cur);
            if (it == parentToChildren.end()) continue;
            for (const auto& child : it->second) {
                if (set.insert(child).second) {
                    stack.push_back(child);
                }
            }
        }
    }

    return { set.begin(), set.end() };
}

bool RenameHandler::memberIsUnambiguous(const std::string& memberName,
                                         const std::string& declaringClass,
                                         std::string& error) const
{
    auto overrideSet = collectOverrideSet(declaringClass);
    std::unordered_set<std::string> okClasses(overrideSet.begin(), overrideSet.end());
    if (okClasses.empty()) okClasses.insert(declaringClass);

    // Scan every open document's symbolLocations. Two rejections:
    //   (a) A "OtherClass.memberName" key whose OtherClass is outside
    //       the override set — cross-class collision within Member kind.
    //   (b) A top-level key == memberName — cross-kind collision with a
    //       top-level function/class. The token-only rename would
    //       rewrite both, which is almost never what the user wants.
    std::string suffix = "." + memberName;
    for (const auto& fileUri : documentManager_->getAllOpenUris()) {
        const Document* doc = documentManager_->getDocument(fileUri);
        if (!doc) continue;
        for (const auto& [key, _] : doc->symbolLocations) {
            if (key == memberName) {
                error = "cannot rename '" + memberName +
                        "': also declared as a top-level symbol "
                        "(rename manually to avoid ambiguous edits)";
                return false;
            }
            if (key.size() <= suffix.size()) continue;
            if (key.compare(key.size() - suffix.size(), suffix.size(), suffix) != 0) {
                continue;
            }
            std::string owner = key.substr(0, key.size() - suffix.size());
            if (okClasses.count(owner) == 0) {
                error = "cannot rename '" + memberName + "': also declared in '" + owner +
                        "' (rename manually to avoid ambiguous edits)";
                return false;
            }
        }
    }
    return true;
}

bool RenameHandler::topLevelIsUnambiguous(const std::string& name,
                                           const std::string& defUri,
                                           std::string& error) const
{
    (void)defUri;  // Not needed — we identify the def doc by symbolLocations
                   // membership rather than URI comparison (URI normalisation
                   // between the visitor's stored URI and findDefinition's
                   // return value is not guaranteed to match exactly).

    // Walk every open document's symbolLocations. Two rejections:
    //   (a) More than one file declares a top-level symbol with this
    //       name — cross-file collision within TopLevel kind.
    //   (b) Any class declares a member with this same name — cross-kind
    //       collision. The token-only rename would rewrite both the
    //       top-level decl and the unrelated method declaration in the
    //       class, which is almost never what the user wants.
    std::string memberSuffix = "." + name;
    int filesWithDecl = 0;
    for (const auto& fileUri : documentManager_->getAllOpenUris()) {
        const Document* doc = documentManager_->getDocument(fileUri);
        if (!doc) continue;
        for (const auto& [key, _] : doc->symbolLocations) {
            if (key == name) {
                filesWithDecl++;
                continue;
            }
            if (key.size() > memberSuffix.size()
                && key.compare(key.size() - memberSuffix.size(),
                               memberSuffix.size(), memberSuffix) == 0) {
                error = "cannot rename '" + name +
                        "': also declared as a class member "
                        "(rename manually to avoid ambiguous edits)";
                return false;
            }
        }
        if (filesWithDecl > 1) {
            error = "cannot rename '" + name + "': also declared as a top-level symbol in another file";
            return false;
        }
    }
    return true;
}

bool RenameHandler::localIsUnambiguous(const Document* doc,
                                        const std::string& name,
                                        std::string& error) const
{
    // Count plausible declaration sites: an IDENTIFIER token of `name`
    // immediately preceded by a type token (primitive type keyword or
    // another IDENTIFIER, which covers `ClassName x` variable decls and
    // typed parameters). Declarations split by whitespace only, so we
    // walk the token list and inspect the previous IDENTIFIER/type token.
    int decls = 0;
    const auto& toks = doc->tokens;
    for (size_t i = 1; i < toks.size(); ++i) {
        const auto& cur = toks[i];
        if (cur.type != token::TokenType::IDENTIFIER) continue;
        if (cur.length != name.size()) continue;
        if (tokenText(doc, cur) != name) continue;
        const auto& prev = toks[i - 1];
        using TT = token::TokenType;
        switch (prev.type) {
            case TT::INT:
            case TT::FLOAT:
            case TT::BOOL:
            case TT::STRING_TYPE:
            case TT::VOID:
            case TT::IDENTIFIER:
            case TT::FINAL:
                decls++;
                break;
            default:
                break;
        }
        if (decls > 1) break;
    }
    if (decls > 1) {
        error = "cannot rename '" + name + "': declared in multiple scopes in this file";
        return false;
    }
    return true;
}

std::vector<Range> RenameHandler::collectOccurrencesInDoc(
    const Document* doc,
    const std::string& name) const
{
    std::vector<Range> out;
    if (!doc) return out;
    for (const auto& tok : doc->tokens) {
        if (tok.type != token::TokenType::IDENTIFIER) continue;
        // Cheap length-prefilter via Token::length (formally safe;
        // stringValue.size() reads a view whose data is dangling here).
        if (tok.length != name.size()) continue;
        if (tokenText(doc, tok) != name) continue;
        out.push_back(tokenRange(tok));
    }
    return out;
}

RenameHandler::PrepareResult RenameHandler::prepareRename(
    const std::string& uri,
    const Position& position)
{
    PrepareResult res;
    TargetInfo info;
    std::string err;
    if (!classifyTarget(uri, position, info, err)) {
        res.ok = false;
        res.error = err;
        return res;
    }
    res.ok = true;
    res.range = info.cursorRange;
    return res;
}

RenameHandler::RenameResult RenameHandler::rename(
    const std::string& uri,
    const Position& position,
    const std::string& newName)
{
    RenameResult res;
    TargetInfo info;
    std::string err;
    if (!classifyTarget(uri, position, info, err)) {
        res.ok = false;
        res.error = err;
        return res;
    }

    if (!isValidIdentifier(newName)) {
        res.ok = false;
        res.error = "'" + newName + "' is not a valid identifier";
        return res;
    }
    if (isReservedKeyword(newName)) {
        res.ok = false;
        res.error = "'" + newName + "' is a reserved keyword";
        return res;
    }
    if (newName == info.name) {
        res.ok = true;  // no-op rename, return empty edit
        return res;
    }

    auto* originDoc = documentManager_->getDocument(uri);
    if (!originDoc) {
        res.ok = false;
        res.error = "document not open";
        return res;
    }

    // Ambiguity gates per kind.
    switch (info.kind) {
        case SymbolKind::TopLevel:
            if (!topLevelIsUnambiguous(info.name, info.defUri, err)) {
                res.ok = false;
                res.error = err;
                return res;
            }
            break;
        case SymbolKind::Member:
            if (!memberIsUnambiguous(info.name, info.declaringClass, err)) {
                res.ok = false;
                res.error = err;
                return res;
            }
            break;
        case SymbolKind::Local:
            if (!localIsUnambiguous(originDoc, info.name, err)) {
                res.ok = false;
                res.error = err;
                return res;
            }
            break;
    }

    // Gather the URI set to scan. Locals stay in the current file;
    // TopLevel/Member range over every open document (the workspace
    // symbol index only tracks files that *declare* top-level symbols,
    // so leaning on it would miss files that only *use* the symbol
    // being renamed).
    std::vector<std::string> uris;
    if (info.kind == SymbolKind::Local) {
        uris.push_back(uri);
    } else {
        std::set<std::string> set;
        set.insert(uri);
        for (const auto& u : documentManager_->getAllOpenUris()) {
            set.insert(u);
        }
        uris.assign(set.begin(), set.end());
    }

    WorkspaceEdit edit;
    for (const auto& u : uris) {
        const Document* d = documentManager_->getDocument(u);
        if (!d) continue;
        auto ranges = collectOccurrencesInDoc(d, info.name);
        if (ranges.empty()) continue;
        // Sort by reverse (line, character) so a client that applies
        // edits in array order does not shift later ranges. The LSP
        // spec leaves apply order unspecified, so be defensive.
        std::sort(ranges.begin(), ranges.end(),
            [](const Range& a, const Range& b) {
                if (a.start.line != b.start.line) return a.start.line > b.start.line;
                return a.start.character > b.start.character;
            });
        std::vector<TextEdit> edits;
        edits.reserve(ranges.size());
        for (const auto& r : ranges) {
            TextEdit te;
            te.range = r;
            te.newText = newName;
            edits.push_back(std::move(te));
        }
        edit.changes[u] = std::move(edits);
    }

    res.ok = true;
    res.edit = std::move(edit);
    return res;
}

} // namespace mtype::lsp
