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
#include "../utils/DeclarationRanges.hpp"

namespace mtype::lsp {

namespace {

// Range helpers live in utils/DeclarationRanges so the workspace type
// graph (MYT-432) reports exactly the ranges the outline shows.
using utils::declRange;
using utils::nameRange;
using utils::singleLineFallback;

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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), sym.name);
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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), sym.name);
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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), "constructor");
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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), sym.name);
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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), sym.name);
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
            child.selectionRange = nameRange(doc->content, doc->tokens, fn->getLocation(), child.name);
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
    sym.selectionRange = nameRange(doc->content, doc->tokens, node->getLocation(), sym.name);
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
