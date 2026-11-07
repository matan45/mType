#include "UnresolvedTypeAnalyzer.hpp"

#include "KnownTypeNames.hpp"
#include "TypeReferenceCollector.hpp"
#include "../DocumentManager.hpp"

#include "../../../mType/diagnostics/DiagnosticBuilder.hpp"
#include "../../../mType/diagnostics/ErrorCodeRegistry.hpp"
#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/environment/registry/FunctionRegistry.hpp"
#include "../../../mType/token/Token.hpp"
#include "../../../mType/token/TokenType.hpp"

#include <string>
#include <unordered_set>

namespace mtype::lsp {

namespace {

using token::TokenType;

// Offsets of the first byte of every line. Lexer lines are 1-based and its
// columns count bytes, so a token's text is content[start(line) + col - 1].
std::vector<std::size_t> lineStarts(const std::string& content) {
    std::vector<std::size_t> starts{0};
    for (std::size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') starts.push_back(i + 1);
    }
    return starts;
}

// The token's source text. Token::stringValue can't be used: it views the
// tokenizing lexer's buffer, which is gone once parsing has finished.
std::string tokenText(const std::string& content, const std::vector<std::size_t>& starts,
                      const token::Token& tok) {
    const int line = tok.location.getLine();
    const int column = tok.location.getColumn();
    if (line < 1 || column < 1 || static_cast<std::size_t>(line) > starts.size()) return {};
    const std::size_t offset = starts[line - 1] + static_cast<std::size_t>(column - 1);
    if (offset + tok.length > content.size()) return {};
    return content.substr(offset, tok.length);
}

// A name after one of these is a member or an annotation, not a type.
bool isMemberPosition(TokenType previous) {
    return previous == TokenType::DOT || previous == TokenType::QUESTION_DOT
        || previous == TokenType::SCOPE || previous == TokenType::AT;
}

bool isEnvironmentFunction(const Document& doc, const std::string& name) {
    auto functions = doc.environment ? doc.environment->getFunctionRegistry() : nullptr;
    return functions && functions->findFunction(name) != nullptr;
}

std::unordered_set<std::string> findUnresolved(const Document& doc) {
    const TypeReferences refs = TypeReferenceCollector::collect(doc.ast);
    const KnownTypeNames known(doc.environment, doc.importSummary, refs);
    std::unordered_set<std::string> unresolved;
    for (const auto& name : refs.referenced) {
        if (known.isKnown(name)) continue;
        if (refs.valueNames.count(name) > 0 || isEnvironmentFunction(doc, name)) continue;
        unresolved.insert(name);
    }
    return unresolved;
}

diagnostics::Diagnostic makeDiagnostic(const token::Token& tok, const std::string& name) {
    const errors::SourceLocation& start = tok.location;
    const errors::SourceLocation end(start.getFilename(), start.getLine(),
                                     start.getColumn() + static_cast<int>(tok.length));
    return diagnostics::DiagnosticBuilder(diagnostics::codes::NameUndefinedClass)
        .withMessage("use of undeclared type '" + name + "'")
        .withPrimaryRange(start, end, "not found in this scope")
        .withNote("import it, or check the spelling")
        .withSourceException("ClassNotFoundException")
        .withData("missingName", name)
        .build();
}

std::vector<diagnostics::Diagnostic> diagnoseTokens(
    const Document& doc, const std::unordered_set<std::string>& unresolved) {
    const auto starts = lineStarts(doc.content);
    std::vector<diagnostics::Diagnostic> out;
    bool inImport = false;
    TokenType previous = TokenType::END; // "nothing before"
    for (const auto& tok : doc.tokens) {
        if (tok.type == TokenType::IMPORT) {
            inImport = true;
        } else if (tok.type == TokenType::SEMICOLON) {
            inImport = false;
        } else if (!inImport && tok.type == TokenType::IDENTIFIER && !isMemberPosition(previous)) {
            const std::string text = tokenText(doc.content, starts, tok);
            if (unresolved.count(text) > 0) out.push_back(makeDiagnostic(tok, text));
        }
        previous = tok.type;
    }
    return out;
}

} // namespace

std::vector<diagnostics::Diagnostic> UnresolvedTypeAnalyzer::analyze(const Document& doc) {
    try {
        if (doc.ast.empty() || doc.tokens.empty()) return {};
        const auto unresolved = findUnresolved(doc);
        if (unresolved.empty()) return {};
        return diagnoseTokens(doc, unresolved);
    } catch (...) {
        return {}; // an analysis bug must never break parsing
    }
}

} // namespace mtype::lsp
