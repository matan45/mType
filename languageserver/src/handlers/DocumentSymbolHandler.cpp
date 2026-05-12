#include "DocumentSymbolHandler.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/errors/SourceLocation.hpp"
#include "../../../mType/token/Token.hpp"
#include "../../../mType/token/TokenType.hpp"

namespace mtype::lsp {

namespace {

using TT = token::TokenType;

// LSP positions are 0-based; SourceLocation is 1-based. Centralize so
// drift between handlers never bites us.
Position fromLoc(const errors::SourceLocation& loc) {
    Position p;
    p.line = loc.getLine() - 1;
    p.character = loc.getColumn() - 1;
    if (p.line < 0) p.line = 0;
    if (p.character < 0) p.character = 0;
    return p;
}

// Compute (line, column) offset (0-based) into doc->content. Returns
// std::string::npos when the position is out of range.
std::size_t offsetOf(const std::string& content, int line, int character) {
    std::size_t pos = 0;
    int currentLine = 0;
    while (currentLine < line && pos < content.size()) {
        if (content[pos] == '\n') currentLine++;
        pos++;
    }
    if (currentLine != line) return std::string::npos;
    std::size_t off = pos + static_cast<std::size_t>(character);
    if (off > content.size()) return std::string::npos;
    return off;
}

// Returns the (line, character) 0-based position for an absolute byte
// offset into `content`.
Position positionFromOffset(const std::string& content, std::size_t offset) {
    Position p;
    p.line = 0;
    p.character = 0;
    if (offset > content.size()) offset = content.size();
    std::size_t lineStart = 0;
    for (std::size_t i = 0; i < offset; ++i) {
        if (content[i] == '\n') {
            p.line++;
            lineStart = i + 1;
        }
    }
    p.character = static_cast<int>(offset - lineStart);
    return p;
}

// Locate the first token at-or-after the given source location. Linear
// scan — documents are small and this only runs per top-level symbol.
std::size_t findTokenIndexAt(const std::vector<token::Token>& toks,
                             const errors::SourceLocation& loc) {
    int line = loc.getLine();
    int col = loc.getColumn();
    for (std::size_t i = 0; i < toks.size(); ++i) {
        int tl = toks[i].location.getLine();
        int tc = toks[i].location.getColumn();
        if (tl > line || (tl == line && tc >= col)) {
            return i;
        }
    }
    return toks.size();
}

// `selectionRange` for a named declaration: scan doc->content forward
// from the node's start location to find the first occurrence of
// `name`. Falls back to a zero-width range at the start position if
// the name can't be located (e.g., stale AST after a destructive edit).
Range nameRange(const std::string& content,
                const errors::SourceLocation& loc,
                const std::string& name) {
    Range r;
    r.start = fromLoc(loc);
    r.end = r.start;
    if (name.empty()) return r;

    std::size_t off = offsetOf(content, r.start.line, r.start.character);
    if (off == std::string::npos) return r;

    auto found = content.find(name, off);
    if (found == std::string::npos) return r;

    r.start = positionFromOffset(content, found);
    r.end = r.start;
    r.end.character += static_cast<int>(name.size());
    return r;
}

// `range` for a declaration: walk doc->tokens forward from the node's
// start location, counting brace depth across the body. Lexer emits
// strings/comments as whole tokens, so this is safe against `}`
// characters inside literals. Handles:
//   - block bodies (class/interface/method/ctor/function): end at the
//     matching RBRACE that closes the body.
//   - abstract methods / interface signatures with no body: end at the
//     SEMICOLON that terminates the signature.
//   - field-style decls with neither LBRACE nor SEMICOLON: degenerate
//     to a single-line range.
// If the walk runs off the end of the token stream (truncated source
// mid-edit), fall back to the last token's end position so the outline
// still has a valid `range` and the editor can highlight up to that
// point.
Range declRange(const std::string& content,
                const std::vector<token::Token>& toks,
                const errors::SourceLocation& loc,
                const Range& fallback) {
    Range r;
    r.start = fromLoc(loc);
    r.end = fallback.end;

    if (toks.empty()) return r;

    std::size_t idx = findTokenIndexAt(toks, loc);
    if (idx >= toks.size()) return r;

    // First pass: find either an LBRACE (block body) or a SEMICOLON
    // (signature-only declaration), whichever comes first.
    std::size_t scan = idx;
    while (scan < toks.size()) {
        TT t = toks[scan].type;
        if (t == TT::LBRACE) break;
        if (t == TT::SEMICOLON) {
            // Single-statement decl — end at the semicolon (inclusive).
            const auto& tok = toks[scan];
            Position end = fromLoc(tok.location);
            end.character += 1;  // SEMICOLON lexeme is one char
            r.end = end;
            return r;
        }
        scan++;
    }
    if (scan >= toks.size()) {
        // No body, no semicolon — leave fallback end in place. Caller
        // typically passes a same-line fallback so the range stays
        // visually localized.
        return r;
    }

    // Brace-depth walk from the first LBRACE to its matching RBRACE.
    int depth = 0;
    for (std::size_t k = scan; k < toks.size(); ++k) {
        TT t = toks[k].type;
        if (t == TT::LBRACE) depth++;
        else if (t == TT::RBRACE) {
            depth--;
            if (depth == 0) {
                const auto& tok = toks[k];
                Position end = fromLoc(tok.location);
                end.character += 1;  // RBRACE lexeme is one char
                r.end = end;
                return r;
            }
        }
    }

    // Unbalanced — source truncated mid-class. Fall back to the last
    // token's end position. Better than collapsing to selectionRange.
    const auto& last = toks.back();
    Position end = fromLoc(last.location);
    end.character += static_cast<int>(last.stringValue.size());
    r.end = end;
    return r;
}

// Single-line fallback range: useful for fields and other
// declarations where declRange might not find a brace or semicolon.
// Spans from the name position to the end of the line.
Range singleLineFallback(const std::string& content, const Range& nameR) {
    Range r;
    r.start = nameR.start;
    r.end = nameR.end;

    std::size_t off = offsetOf(content, nameR.start.line, nameR.start.character);
    if (off == std::string::npos) return r;
    while (off < content.size() && content[off] != '\n') off++;
    r.end = positionFromOffset(content, off);
    return r;
}

// Build a comma-free modifier-prefix detail string. e.g., "static async".
std::string joinModifiers(std::initializer_list<std::pair<bool, const char*>> mods) {
    std::string out;
    for (const auto& [present, label] : mods) {
        if (!present) continue;
        if (!out.empty()) out += ' ';
        out += label;
    }
    return out;
}

DocumentSymbol buildField(const ast::nodes::classes::FieldNode* node,
                          const Document* doc) {
    DocumentSymbol sym;
    sym.name = node->getName();
    sym.kind = SymbolKind::Field;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), sym.name);
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);

    std::string mods = joinModifiers({
        {node->getIsStatic(), "static"},
        {node->getIsFinal(), "final"}
    });
    if (!mods.empty()) sym.detail = mods;
    return sym;
}

DocumentSymbol buildMethod(const ast::nodes::classes::MethodNode* node,
                           const Document* doc) {
    DocumentSymbol sym;
    sym.name = node->getName();
    sym.kind = SymbolKind::Method;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), sym.name);
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);

    std::string mods = joinModifiers({
        {node->getIsStatic(), "static"},
        {node->getIsAsync(), "async"},
        {node->isAbstract(), "abstract"},
        {node->isFinal(), "final"}
    });
    std::string arity = "(" + std::to_string(node->getParameterCount()) + ")";
    sym.detail = mods.empty() ? arity : mods + " " + arity;
    return sym;
}

DocumentSymbol buildConstructor(const ast::nodes::classes::ConstructorNode* node,
                                const Document* doc,
                                const std::string& className) {
    DocumentSymbol sym;
    sym.name = className;
    sym.kind = SymbolKind::Constructor;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), "constructor");
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);
    sym.detail = "(" + std::to_string(node->getParameterCount()) + ")";
    return sym;
}

DocumentSymbol buildClass(const ast::nodes::classes::ClassNode* node,
                          const Document* doc) {
    DocumentSymbol sym;
    sym.name = node->getClassName();
    sym.kind = SymbolKind::Class;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), sym.name);
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);

    std::string mods = joinModifiers({
        {node->isAbstract(), "abstract"},
        {node->isFinal(), "final"},
        {node->isValueClass(), "value"}
    });
    if (!mods.empty()) sym.detail = mods;

    // Children in source order: constructors → fields → methods. (The
    // parser keeps each list separately, but rebuilding their original
    // interleaving would require re-scanning the token stream. For an
    // outline view, grouped-by-kind reads better anyway.)
    for (const auto& ctor : node->getConstructors()) {
        if (auto* c = dynamic_cast<ast::nodes::classes::ConstructorNode*>(ctor.get())) {
            sym.children.push_back(buildConstructor(c, doc, sym.name));
        }
    }
    for (const auto& field : node->getFields()) {
        if (auto* f = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get())) {
            sym.children.push_back(buildField(f, doc));
        }
    }
    for (const auto& method : node->getMethods()) {
        if (auto* m = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get())) {
            sym.children.push_back(buildMethod(m, doc));
        }
    }
    return sym;
}

DocumentSymbol buildInterface(const ast::nodes::classes::InterfaceNode* node,
                              const Document* doc) {
    DocumentSymbol sym;
    sym.name = node->getName();
    sym.kind = SymbolKind::Interface;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), sym.name);
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);

    if (node->isFinal()) sym.detail = std::string("final");

    // Interface methods are stored as FunctionNode signatures in the
    // AST, but for the outline we render them as Method symbols since
    // they live under an Interface scope.
    for (const auto& method : node->getMethods()) {
        const ast::ASTNode* m = method.get();
        if (auto* fn = dynamic_cast<const ast::nodes::functions::FunctionNode*>(m)) {
            DocumentSymbol child;
            child.name = fn->getName();
            child.kind = SymbolKind::Method;
            child.selectionRange = nameRange(doc->content, fn->getLocation(), child.name);
            Range childFallback = singleLineFallback(doc->content, child.selectionRange);
            child.range = declRange(doc->content, doc->tokens, fn->getLocation(), childFallback);
            std::string arity = "(" + std::to_string(fn->getParameterCount()) + ")";
            child.detail = fn->getIsAsync() ? std::string("async ") + arity : arity;
            sym.children.push_back(std::move(child));
        } else if (auto* mn = dynamic_cast<const ast::nodes::classes::MethodNode*>(m)) {
            // Defensive — some interface parsers may emit MethodNodes.
            sym.children.push_back(buildMethod(mn, doc));
        }
    }
    return sym;
}

DocumentSymbol buildFunction(const ast::nodes::functions::FunctionNode* node,
                             const Document* doc) {
    DocumentSymbol sym;
    sym.name = node->getName();
    sym.kind = SymbolKind::Function;
    sym.selectionRange = nameRange(doc->content, node->getLocation(), sym.name);
    Range fallback = singleLineFallback(doc->content, sym.selectionRange);
    sym.range = declRange(doc->content, doc->tokens, node->getLocation(), fallback);

    std::string arity = "(" + std::to_string(node->getParameterCount()) + ")";
    sym.detail = node->getIsAsync() ? std::string("async ") + arity : arity;
    return sym;
}

} // namespace

DocumentSymbolHandler::DocumentSymbolHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::vector<DocumentSymbol> DocumentSymbolHandler::handleDocumentSymbol(
    const std::string& uri)
{
    std::vector<DocumentSymbol> out;

    auto* doc = documentManager_->getDocument(uri);
    if (!doc || !doc->isParsed || doc->ast.empty()) {
        return out;
    }

    try {
        auto* program = dynamic_cast<ast::nodes::statements::ProgramNode*>(
            doc->ast[0].get());
        if (!program) return out;

        for (const auto& stmt : program->getStatements()) {
            ast::ASTNode* node = stmt.get();
            if (!node) continue;

            if (auto* cls = dynamic_cast<ast::nodes::classes::ClassNode*>(node)) {
                out.push_back(buildClass(cls, doc));
            } else if (auto* iface = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node)) {
                out.push_back(buildInterface(iface, doc));
            } else if (auto* fn = dynamic_cast<ast::nodes::functions::FunctionNode*>(node)) {
                out.push_back(buildFunction(fn, doc));
            }
            // Unknown top-level statements (imports, top-level expressions,
            // AssignmentNode-style variable decls) are skipped silently —
            // see header for the rationale on top-level variables.
        }
    } catch (...) {
        // Any per-document failure → empty symbol list. Better no outline
        // than a crashed LSP response.
        out.clear();
    }
    return out;
}

} // namespace mtype::lsp
