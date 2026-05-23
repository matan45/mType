#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

// MYT-295 — Inlay hints for the mType LSP.
//
// Two hint flavours, both conservative by design:
//   1. Parameter-name hints at user-defined function, method, and
//      constructor call sites. Built-ins (print, hashCode, etc.) are
//      filtered out so the editor doesn't get visually noisy on every
//      print(); overloaded calls (ambiguous resolution) are skipped.
//      A hint is suppressed when the argument is a bare identifier
//      whose lexeme already matches the parameter name — the LSP
//      spec's "no hint where it duplicates source text" rule.
//   2. Type hints on lambda parameters that lack an explicit type
//      annotation. mType requires explicit types on every variable
//      declaration, so this is the only place an identifier has no
//      source-level type. The hint label comes from the target
//      functional interface's SAM signature; if the target isn't a
//      resolvable functional interface, no hint is emitted.
//
// The handler walks the document's token stream rather than the AST.
// On partial / unparsed source it returns an empty vector — never
// throws — so an edit mid-keystroke degrades to "no hints" cleanly.
class InlayHintHandler {
public:
    explicit InlayHintHandler(DocumentManager* docMgr);

    std::vector<InlayHint> handleInlayHint(
        const std::string& uri,
        const Range& range);

private:
    struct CallArg {
        Position startPos;          // LSP-0-based position of the first token of the arg
        bool isBareIdentifier = false;
        std::string identifierText; // Lexeme when isBareIdentifier; empty otherwise
    };

    enum class CalleeKind { Function, Method, Constructor, StaticMethod };

    // A pending call site discovered during the token walk; resolved
    // to parameter names in a second pass.
    struct CallSite {
        CalleeKind kind;
        std::string calleeName;       // function/method/class name
        std::string receiverName;     // for Method: the lexeme to the left of '.'
        int callLine = 0;             // 0-based line of the LPAREN token
        std::vector<CallArg> args;
    };

    struct LambdaParam {
        Position endPos;     // position immediately AFTER the parameter identifier
        std::string name;    // lexeme of the parameter identifier
    };

    // A pending lambda site discovered during the token walk.
    struct LambdaSite {
        std::vector<LambdaParam> params;  // only the untyped ones
        std::string targetInterface;      // resolved from the lexical context, may be empty
    };

    // Map of variable name → declared class name, populated in pass 1.
    // Replaces an earlier O(N×M) per-call token scan: the rule "first
    // `IDENTIFIER IDENTIFIER` pair where the second is the var name"
    // can be answered once by sweeping the token list, so we do.
    using ReceiverTypeMap = std::unordered_map<std::string, std::string>;

    // Pass 1 — extract call sites and lambda sites from the token stream.
    void collectSites(
        const Document* doc,
        const Range& range,
        std::vector<CallSite>& calls,
        std::vector<LambdaSite>& lambdas,
        ReceiverTypeMap& receiverTypes) const;

    // Pass 2a — for each call site, resolve parameter names and emit hints.
    void emitParameterHints(
        const Document* doc,
        const std::vector<CallSite>& calls,
        const ReceiverTypeMap& receiverTypes,
        std::vector<InlayHint>& out) const;

    // Pass 2b — for each lambda, resolve target interface SAM types and emit hints.
    void emitLambdaTypeHints(
        const Document* doc,
        const std::vector<LambdaSite>& lambdas,
        std::vector<InlayHint>& out) const;

    // Suppression rule: bare identifier whose text equals the parameter name.
    static bool argumentDuplicatesParamName(
        const CallArg& arg, const std::string& paramName);

    // mType built-in functions get no parameter hints (user choice — keeps
    // print(...) etc. visually clean). List mirrors
    // SignatureHelpHandler::getBuiltInSignature.
    static bool isBuiltInName(const std::string& name);

    static bool inRange(const Position& p, const Range& r);
    static Position tokenStart(const token::Token& tok);
    static Position tokenEnd(const token::Token& tok);

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
