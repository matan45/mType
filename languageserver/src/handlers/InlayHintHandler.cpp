#include "InlayHintHandler.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_set>

#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/environment/registry/FunctionRegistry.hpp"
#include "../../../mType/environment/registry/FunctionDefinition.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/ConstructorDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"
#include "../../../mType/token/TokenType.hpp"
#include "../../../mType/types/UnifiedType.hpp"
#include "../utils/LSPUtils.hpp"

namespace mtype::lsp {

namespace {

using TT = token::TokenType;
using utils::tokenText;

// Walk-back boundary tokens. When walking backwards from a lambda's
// param-list start to find its enclosing `= <Interface> <var>`, hitting
// any of these means we left the immediate `<Interface> <var> = lambda`
// context. Includes LPAREN/COMMA so a lambda passed as a function
// argument (e.g., `f(x -> x * 2)`) does not pick up the prior
// statement's `=` as its assignment target.
bool isWalkBackBoundary(TT t) {
    return t == TT::SEMICOLON || t == TT::LBRACE || t == TT::RBRACE
        || t == TT::LPAREN || t == TT::RPAREN
        || t == TT::LBRACKET || t == TT::RBRACKET
        || t == TT::COMMA;
}

// MYT-354 — for a `return <lambda>` whose walk-back hit RETURN, find the
// enclosing function's declared return-type identifier so it can serve
// as the lambda's target interface. Token-stream only — keeps the
// handler stable on partially-parsed source.
std::string resolveEnclosingFunctionReturnType(
    const Document* doc, std::size_t fromIdx)
{
    const auto& toks = doc->tokens;
    // Walk LEFT from RETURN, brace-balancing. We must pass through
    // enclosing block openers (if/while/function body) until we reach
    // the enclosing `function` keyword at depth 0.
    int braceDepth = 0;
    std::size_t fnIdx = 0;
    bool foundFn = false;
    for (std::size_t r = fromIdx; r > 0;) {
        r--;
        if (toks[r].type == TT::RBRACE) {
            braceDepth++;
        } else if (toks[r].type == TT::LBRACE) {
            if (braceDepth > 0) braceDepth--;
            // else: crossing an enclosing block opener — keep going.
        } else if (toks[r].type == TT::FUNCTION && braceDepth == 0) {
            fnIdx = r;
            foundFn = true;
            break;
        }
    }
    if (!foundFn) return "";
    // From `function`, scan forward, paren-balancing, until the first
    // LBRACE at paren depth 0 that follows at least one matched RPAREN.
    // That LBRACE is the function body opener.
    int parenDepth = 0;
    bool sawRparen = false;
    std::size_t lb = 0;
    for (std::size_t s = fnIdx + 1; s < toks.size(); ++s) {
        if (toks[s].type == TT::LPAREN) parenDepth++;
        else if (toks[s].type == TT::RPAREN) {
            parenDepth--;
            if (parenDepth == 0) sawRparen = true;
        } else if (toks[s].type == TT::LBRACE && parenDepth == 0 && sawRparen) {
            lb = s;
            break;
        }
    }
    if (lb == 0) return "";
    // Inspect tokens immediately before LBRACE: pattern `: IDENTIFIER [<…>] {`.
    std::size_t p = lb;
    // Strip a trailing generic argument list: `…<X, Y>` → `…`.
    if (p > 0 && toks[p - 1].type == TT::GREATER) {
        int gd = 1;
        std::size_t g = p - 1;
        while (g > 0 && gd > 0) {
            g--;
            if (toks[g].type == TT::GREATER) gd++;
            else if (toks[g].type == TT::LESS) gd--;
        }
        if (gd != 0) return "";
        p = g; // now points at LESS
    }
    if (p >= 2 && toks[p - 1].type == TT::IDENTIFIER
        && toks[p - 2].type == TT::COLON) {
        return tokenText(doc, toks[p - 1]);
    }
    return "";  // void function or unrecognized signature shape
}

// MYT-363 — return-type-name helpers. Each mirrors its lookupXParams
// sibling but returns the bare class/interface name of the call's
// declared return type, so a chained `.method(...)` in
// `recv.head(...).method(...)` can look up `method` on the head's
// return-type class. Array wrappers are peeled (matches the
// ReceiverTypeResolver convention) so `Stream[]` resolves to `Stream`.
// Defined here (early in the file) so collectSites can call them when
// closing each call's frame.
std::string baseTypeName(const ::types::UnifiedTypePtr& ut) {
    if (!ut) return {};
    ::types::UnifiedTypePtr cur = ut;
    while (cur && cur->isArray()) {
        cur = cur->getElementType();
    }
    if (!cur) return {};
    return cur->getName();
}

std::string lookupFunctionReturnTypeName(
    const Document* doc, const std::string& name)
{
    if (!doc || !doc->environment) return {};
    auto reg = doc->environment->getFunctionRegistry();
    if (!reg) return {};
    auto fn = reg->findFunction(name);
    if (!fn) return {};
    return baseTypeName(fn->getUnifiedReturnType());
}

std::string lookupMethodReturnTypeName(
    const Document* doc, const std::string& className,
    const std::string& methodName)
{
    if (!doc || !doc->environment) return {};

    if (auto creg = doc->environment->getClassRegistry()) {
        if (auto cls = creg->findClass(className)) {
            if (auto method = cls->getMethod(methodName)) {
                return baseTypeName(method->getUnifiedReturnType());
            }
        }
    }
    // Interface fallback — mirrors lookupMethodParams.
    if (auto iface = doc->environment->findInterface(className)) {
        for (const auto& sig : iface->getMethodSignatures()) {
            if (sig.name == methodName) {
                return baseTypeName(sig.returnType);
            }
        }
    }
    return {};
}

std::string lookupStaticMethodReturnTypeName(
    const Document* doc, const std::string& className,
    const std::string& methodName)
{
    if (!doc || !doc->environment) return {};
    auto creg = doc->environment->getClassRegistry();
    if (!creg) return {};
    auto cls = creg->findClass(className);
    if (!cls) return {};
    auto method = cls->getStaticMethod(methodName);
    if (!method) return {};
    return baseTypeName(method->getUnifiedReturnType());
}

} // namespace

InlayHintHandler::InlayHintHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

bool InlayHintHandler::isBuiltInName(const std::string& name) {
    // Mirrors SignatureHelpHandler::getBuiltInSignature. Per MYT-295
    // user choice, built-ins do not receive parameter-name hints.
    static const std::unordered_set<std::string> kBuiltins = {
        "print", "hashCode", "strLength", "parsePrimitive"
    };
    return kBuiltins.count(name) > 0;
}

bool InlayHintHandler::inRange(const Position& p, const Range& r) {
    if (p.line < r.start.line || p.line > r.end.line) return false;
    if (p.line == r.start.line && p.character < r.start.character) return false;
    if (p.line == r.end.line && p.character > r.end.character) return false;
    return true;
}

Position InlayHintHandler::tokenStart(const token::Token& tok) {
    Position p;
    p.line = tok.location.getLine() - 1;
    p.character = tok.location.getColumn() - 1;
    return p;
}

Position InlayHintHandler::tokenEnd(const token::Token& tok) {
    Position p = tokenStart(tok);
    p.character += static_cast<int>(tok.length);
    return p;
}

bool InlayHintHandler::argumentDuplicatesParamName(
    const CallArg& arg, const std::string& paramName) {
    return arg.isBareIdentifier && arg.identifierText == paramName;
}

std::vector<InlayHint> InlayHintHandler::handleInlayHint(
    const std::string& uri,
    const Range& range)
{
    std::vector<InlayHint> out;

    auto* doc = documentManager_->getDocument(uri);
    // Stable-while-editing rule: any unparseable / not-yet-parsed
    // state degrades cleanly to "no hints". Never throws.
    if (!doc || !doc->isParsed || doc->tokens.empty()) {
        return out;
    }

    try {
        std::vector<CallSite> calls;
        std::vector<LambdaSite> lambdas;
        ReceiverTypeMap receiverTypes;
        collectSites(doc, range, calls, lambdas, receiverTypes);
        emitParameterHints(doc, calls, receiverTypes, out);
        emitLambdaTypeHints(doc, lambdas, out);
    } catch (...) {
        // Any per-document failure → empty hint list. Better no hints
        // than a crashed LSP response.
        out.clear();
    }
    return out;
}

void InlayHintHandler::collectSites(
    const Document* doc,
    const Range& range,
    std::vector<CallSite>& calls,
    std::vector<LambdaSite>& lambdas,
    ReceiverTypeMap& receiverTypes) const
{
    const auto& toks = doc->tokens;

    // MYT-363 — chained-call return-type tracking. Keyed by the RPAREN
    // token index of a closed call site, holds the bare class/interface
    // name of that call's declared return type. Lets us classify the
    // next link in a `.method(...).method(...)` chain by treating the
    // closed call's return type as the receiver class.
    std::unordered_map<std::size_t, std::string> closedReturnTypes;

    // Build the variable-name → declared-class-name map in this same
    // pass. Rule mirrors the old resolveReceiverClassName: take the
    // FIRST `IDENTIFIER IDENTIFIER` pair we see for a given var name.
    // try_emplace keeps the first binding so subsequent shadowing
    // (later in the file) does not overwrite earlier visible scopes.
    for (std::size_t i = 1; i < toks.size(); ++i) {
        if (toks[i].type != TT::IDENTIFIER) continue;
        if (toks[i - 1].type != TT::IDENTIFIER) continue;
        std::string varName = tokenText(doc, toks[i]);
        if (varName.empty()) continue;
        receiverTypes.try_emplace(std::move(varName), tokenText(doc, toks[i - 1]));
    }

    // Stack frame for one open '('. Tracks the in-flight argument so
    // we can emit one CallArg per top-level comma and one at RPAREN.
    struct Frame {
        bool isCall = false;
        CallSite call;
        std::size_t argFirstTokIdx = 0;  // index of the first token of the current arg
        std::size_t argTokenCount = 0;
        int innerNesting = 0;  // (, [, { depth INSIDE this paren frame
    };

    std::vector<Frame> stack;

    auto closeArg = [&](Frame& f, std::size_t commaOrRparenIdx) {
        (void)commaOrRparenIdx;
        if (f.argTokenCount == 0) return;  // empty arg slot (e.g., `f()` or trailing)
        CallArg arg;
        const auto& firstTok = toks[f.argFirstTokIdx];
        arg.startPos = tokenStart(firstTok);
        if (f.argTokenCount == 1 && firstTok.type == TT::IDENTIFIER) {
            arg.isBareIdentifier = true;
            arg.identifierText = tokenText(doc, firstTok);
        }
        f.call.args.push_back(std::move(arg));
    };

    for (std::size_t i = 0; i < toks.size(); ++i) {
        const auto& cur = toks[i];

        // Lambda detection — ARROW with optional paren'd param list to its left.
        if (cur.type == TT::ARROW) {
            LambdaSite lam;
            // paramListStart points at the FIRST token of the param list:
            //   - case (a) bare IDENTIFIER: the identifier itself.
            //   - case (b) `(...) ->`     : the LPAREN.
            // Used below to start the walk-back search for the enclosing
            // `= <Interface> <var>` *outside* the param list.
            std::size_t paramListStart = i;
            // Locate the start of the param list. Two cases:
            //   a) `IDENTIFIER ARROW`   — single untyped param.
            //   b) `RPAREN ... LPAREN ARROW` — paren'd param list (may be empty,
            //      may be typed or untyped).
            if (i >= 1 && toks[i - 1].type == TT::IDENTIFIER) {
                // Case (a). Check the token BEFORE the identifier: if it's
                // RPAREN or any type token, this isn't a fresh untyped param —
                // skip silently. The common shape is just `x ->` where the
                // preceding token is `=` / `(` / `,`.
                std::string name = tokenText(doc, toks[i - 1]);
                if (!name.empty()) {
                    LambdaParam p;
                    p.endPos = tokenEnd(toks[i - 1]);
                    p.name = name;
                    lam.params.push_back(std::move(p));
                }
                paramListStart = i - 1;
            } else if (i >= 1 && toks[i - 1].type == TT::RPAREN) {
                // Case (b). Walk back from the RPAREN to its matching LPAREN.
                int depth = 1;
                std::size_t j = i - 1;
                while (j > 0 && depth > 0) {
                    j--;
                    if (toks[j].type == TT::RPAREN) depth++;
                    else if (toks[j].type == TT::LPAREN) depth--;
                }
                if (depth == 0 && toks[j].type == TT::LPAREN) {
                    paramListStart = j;
                    // tokens (j+1 .. i-2) are inside the param list. Walk
                    // forward, emit one untyped LambdaParam per IDENTIFIER
                    // that has no preceding type token in its slot.
                    std::size_t paramStart = j + 1;
                    std::size_t end = i - 1;
                    std::size_t k = paramStart;
                    while (k < end) {
                        // A slot starts at paramStart or just after a comma.
                        // Skip leading whitespace-equivalents — there are
                        // none in a clean token stream, but defensively look
                        // for an IDENTIFIER as the slot's single token.
                        // If the first token of this slot is a type token
                        // (primitive keyword or an IDENTIFIER followed by
                        // another IDENTIFIER), this param is typed → skip.
                        if (toks[k].type == TT::COMMA) { k++; continue; }
                        if (toks[k].type == TT::IDENTIFIER) {
                            bool isTyped = false;
                            // Typed shape: IDENTIFIER IDENTIFIER (type-then-name).
                            if (k + 1 < end
                                && toks[k + 1].type == TT::IDENTIFIER) {
                                isTyped = true;
                            }
                            if (!isTyped) {
                                std::string name = tokenText(doc, toks[k]);
                                if (!name.empty()) {
                                    LambdaParam p;
                                    p.endPos = tokenEnd(toks[k]);
                                    p.name = name;
                                    lam.params.push_back(std::move(p));
                                }
                            }
                            // Skip to the next comma (or end).
                            while (k < end && toks[k].type != TT::COMMA) k++;
                            continue;
                        }
                        if (toks[k].type == TT::INT || toks[k].type == TT::FLOAT
                            || toks[k].type == TT::BOOL || toks[k].type == TT::STRING_TYPE
                            || toks[k].type == TT::VOID) {
                            // Typed slot — skip to comma.
                            while (k < end && toks[k].type != TT::COMMA) k++;
                            continue;
                        }
                        k++;
                    }
                }
            }
            if (!lam.params.empty()) {
                // Range filter — keep the lambda only if at least one param
                // falls inside the requested range.
                bool inWindow = false;
                for (const auto& p : lam.params) {
                    if (inRange(p.endPos, range)) { inWindow = true; break; }
                }
                if (inWindow) {
                    // Resolve target interface by walking back from the
                    // first token of the param list. Two recognised hits:
                    //   - ASSIGN: shape `<InterfaceName> <varName> = <lambda>`.
                    //   - RETURN (MYT-354): shape `return <lambda>` —
                    //     target interface is the enclosing function's
                    //     declared return type.
                    // Hitting any walk-back boundary first means this
                    // lambda isn't recognised — leave targetInterface empty.
                    std::size_t w = paramListStart;
                    bool foundAssign = false;
                    bool foundReturn = false;
                    std::size_t returnIdx = 0;
                    while (w > 0) {
                        w--;
                        if (toks[w].type == TT::ASSIGN) { foundAssign = true; break; }
                        if (toks[w].type == TT::RETURN) {
                            foundReturn = true; returnIdx = w; break;
                        }
                        if (isWalkBackBoundary(toks[w].type)) break;
                    }
                    if (foundAssign && w >= 2
                        && toks[w - 1].type == TT::IDENTIFIER
                        && toks[w - 2].type == TT::IDENTIFIER) {
                        lam.targetInterface = tokenText(doc, toks[w - 2]);
                    } else if (foundReturn) {
                        lam.targetInterface =
                            resolveEnclosingFunctionReturnType(doc, returnIdx);
                    }
                    lambdas.push_back(std::move(lam));
                }
            }
            // ARROW is its own token — continue processing for call frames.
        }

        // Call frame management.
        if (cur.type == TT::LPAREN) {
            // The LPAREN itself is part of the parent frame's current arg.
            // Counting it ensures that `f(g(1))` is not misread as `f`'s
            // arg being the bare identifier `g` — the bare-identifier
            // suppression rule must only fire on a single IDENTIFIER token.
            if (!stack.empty()) {
                stack.back().argTokenCount++;
            }
            Frame f;
            f.argFirstTokIdx = i + 1;
            f.argTokenCount = 0;
            f.innerNesting = 0;

            // Classify whether this LPAREN opens a real call site that we
            // want to hint. Look at the token immediately to the left.
            if (i >= 1 && toks[i - 1].type == TT::IDENTIFIER) {
                // Reject declarations: `function NAME (...)` and the parens
                // following a parameter type in a method declaration. Check
                // the token two-back from the LPAREN.
                bool isDecl = (i >= 2 && toks[i - 2].type == TT::FUNCTION);
                if (!isDecl) {
                    f.isCall = true;
                    f.call.callLine = toks[i - 1].location.getLine() - 1;
                    f.call.calleeName = tokenText(doc, toks[i - 1]);

                    // Classify call kind.
                    if (i >= 2 && toks[i - 2].type == TT::DOT
                        && i >= 3 && toks[i - 3].type == TT::IDENTIFIER) {
                        f.call.kind = CalleeKind::Method;
                        f.call.receiverName = tokenText(doc, toks[i - 3]);
                    } else if (i >= 2 && toks[i - 2].type == TT::NEW) {
                        f.call.kind = CalleeKind::Constructor;
                    } else if (i >= 2 && toks[i - 2].type == TT::SCOPE
                               && i >= 3 && toks[i - 3].type == TT::IDENTIFIER) {
                        // MYT-355 — static call `ClassName::method(...)`.
                        // The receiver token IS the class name, not a
                        // variable, so no receiverTypes lookup is needed
                        // in emitParameterHints.
                        f.call.kind = CalleeKind::StaticMethod;
                        f.call.receiverName = tokenText(doc, toks[i - 3]);
                    } else if (i >= 2 && toks[i - 2].type == TT::DOT) {
                        // MYT-363 — chained method call. The token shape is
                        // `RPAREN DOT IDENTIFIER LPAREN`, i.e. `.method(` on
                        // the result of a previous call. If the previous
                        // call's RPAREN recorded a return-type class name,
                        // use it as the pre-resolved receiver class for
                        // this link. Anything else (parenthesized
                        // expression, ternary, subscript) still bails.
                        bool resolved = false;
                        if (i >= 3 && toks[i - 3].type == TT::RPAREN) {
                            auto it = closedReturnTypes.find(i - 3);
                            if (it != closedReturnTypes.end()
                                && !it->second.empty()) {
                                f.call.kind = CalleeKind::Method;
                                f.call.receiverClassName = it->second;
                                resolved = true;
                            }
                        }
                        if (!resolved) {
                            f.isCall = false;
                        }
                    } else {
                        f.call.kind = CalleeKind::Function;
                    }
                }
            }
            stack.push_back(std::move(f));
            continue;
        }

        if (!stack.empty()) {
            Frame& top = stack.back();
            // Track inner nesting so commas inside `[...]`, `{...}`, or a
            // nested `(...)` don't break arg-splitting on this frame.
            if (cur.type == TT::LBRACKET || cur.type == TT::LBRACE) {
                top.innerNesting++;
            } else if (cur.type == TT::RBRACKET || cur.type == TT::RBRACE) {
                if (top.innerNesting > 0) top.innerNesting--;
            } else if (cur.type == TT::COMMA && top.innerNesting == 0) {
                closeArg(top, i);
                top.argFirstTokIdx = i + 1;
                top.argTokenCount = 0;
                continue;
            } else if (cur.type == TT::RPAREN) {
                if (top.innerNesting > 0) {
                    top.innerNesting--;
                } else {
                    // Closing this frame.
                    closeArg(top, i);
                    if (top.isCall) {
                        // MYT-363 — before we move the call out, record its
                        // declared return-type class name keyed by this
                        // RPAREN's index. The next `.method(...)` in a
                        // chain (if any) reads this map to resolve its
                        // receiver class. Recorded regardless of range
                        // filtering — a chained link inside the range may
                        // depend on a head call outside it.
                        std::string retType;
                        switch (top.call.kind) {
                            case CalleeKind::Function:
                                retType = lookupFunctionReturnTypeName(
                                    doc, top.call.calleeName);
                                break;
                            case CalleeKind::Constructor:
                                // `new Foo(...)` returns Foo.
                                retType = top.call.calleeName;
                                break;
                            case CalleeKind::Method: {
                                std::string cls = top.call.receiverClassName;
                                if (cls.empty()) {
                                    auto it = receiverTypes.find(top.call.receiverName);
                                    if (it != receiverTypes.end()) cls = it->second;
                                }
                                if (!cls.empty()) {
                                    retType = lookupMethodReturnTypeName(
                                        doc, cls, top.call.calleeName);
                                }
                                break;
                            }
                            case CalleeKind::StaticMethod:
                                retType = lookupStaticMethodReturnTypeName(
                                    doc, top.call.receiverName, top.call.calleeName);
                                break;
                        }
                        if (!retType.empty()) {
                            closedReturnTypes[i] = std::move(retType);
                        }

                        // Range filter — keep call sites whose LPAREN is in window.
                        // top.call.callLine is the line of the callee identifier;
                        // use the first arg's pos (or the LPAREN line) for filter.
                        Position callPos;
                        callPos.line = top.call.callLine;
                        callPos.character = 0;
                        // Generous: include if any arg's start is in window
                        // OR the call line itself overlaps the range.
                        bool inWindow = (callPos.line >= range.start.line
                                         && callPos.line <= range.end.line);
                        if (!inWindow) {
                            for (const auto& a : top.call.args) {
                                if (inRange(a.startPos, range)) { inWindow = true; break; }
                            }
                        }
                        if (inWindow) {
                            calls.push_back(std::move(top.call));
                        }
                    }
                    stack.pop_back();
                    continue;
                }
            }
            // Any other token contributes to the current arg.
            top.argTokenCount++;
        }
    }
}

namespace {

// Resolve a top-level function's parameter names from doc's environment.
std::vector<std::string> lookupFunctionParams(
    const Document* doc, const std::string& name)
{
    std::vector<std::string> out;
    if (!doc || !doc->environment) return out;
    auto reg = doc->environment->getFunctionRegistry();
    if (!reg) return out;
    auto fn = reg->findFunction(name);
    if (!fn) return out;
    for (const auto& [pname, _ptype] : fn->getParameters()) {
        out.push_back(pname);
    }
    return out;
}

// Resolve a method's parameter names. MethodDefinition::getParameters()
// places `this` first for instance methods — drop it so positional
// arguments align.
// MYT-356 — when the receiver's declared type is an interface, the
// class registry misses; fall back to the interface registry. Interface
// methods carry no implicit `this`, so the leading-param skip doesn't
// apply on that path.
std::vector<std::string> lookupMethodParams(
    const Document* doc, const std::string& className,
    const std::string& methodName)
{
    std::vector<std::string> out;
    if (!doc || !doc->environment) return out;

    // Try class registry first.
    if (auto creg = doc->environment->getClassRegistry()) {
        if (auto cls = creg->findClass(className)) {
            if (auto method = cls->getMethod(methodName)) {
                bool first = true;
                for (const auto& [pname, _ptype] : method->getParameters()) {
                    if (first && pname == "this") { first = false; continue; }
                    first = false;
                    out.push_back(pname);
                }
                return out;
            }
        }
    }

    // Interface fallback. Scan by name; first overload wins, mirroring
    // the class path's getMethod(name) semantics.
    if (auto iface = doc->environment->findInterface(className)) {
        for (const auto& sig : iface->getMethodSignatures()) {
            if (sig.name == methodName) {
                for (const auto& [pname, _ptype] : sig.parameters) {
                    out.push_back(pname);
                }
                return out;
            }
        }
    }
    return out;
}

// MYT-355 — resolve a static method's parameter names. Static methods
// have no implicit `this`, so no leading-param skip is needed.
std::vector<std::string> lookupStaticMethodParams(
    const Document* doc, const std::string& className,
    const std::string& methodName)
{
    std::vector<std::string> out;
    if (!doc || !doc->environment) return out;
    auto creg = doc->environment->getClassRegistry();
    if (!creg) return out;
    auto cls = creg->findClass(className);
    if (!cls) return out;
    auto method = cls->getStaticMethod(methodName);
    if (!method) return out;
    for (const auto& [pname, _ptype] : method->getParameters()) {
        out.push_back(pname);
    }
    return out;
}

// Extract parameter NAMES from a token-level `(...)` parameter list.
// The list starts at toks[lparenIdx] (an LPAREN) and ends at the matching
// RPAREN. Each slot is `<type tokens...> <identifier-name>` (possibly
// with generic angle brackets). The param NAME is the LAST IDENTIFIER
// token in the slot, which is robust to both primitive-typed
// (`int width`) and class-typed (`Box outer`, `List<int> xs`) params.
std::vector<std::string> parseParamNamesFromTokenList(
    const Document* doc,
    const std::vector<token::Token>& toks,
    std::size_t lparenIdx)
{
    std::vector<std::string> names;
    if (lparenIdx >= toks.size() || toks[lparenIdx].type != TT::LPAREN) {
        return names;
    }
    std::size_t i = lparenIdx + 1;
    std::size_t slotStart = i;
    int parenDepth = 1;
    int angleDepth = 0;

    auto finishSlot = [&](std::size_t slotEnd) {
        // Empty slot (e.g., trailing comma) — nothing to record.
        if (slotEnd <= slotStart) return;
        // Walk back through the slot for the last IDENTIFIER token.
        for (std::size_t k = slotEnd; k > slotStart; --k) {
            if (toks[k - 1].type == TT::IDENTIFIER) {
                names.push_back(tokenText(doc, toks[k - 1]));
                return;
            }
        }
    };

    while (i < toks.size() && parenDepth > 0) {
        const auto t = toks[i].type;
        if (t == TT::LPAREN) {
            parenDepth++;
        } else if (t == TT::RPAREN) {
            parenDepth--;
            if (parenDepth == 0) {
                finishSlot(i);
                break;
            }
        } else if (t == TT::LESS) {
            angleDepth++;
        } else if (t == TT::GREATER) {
            if (angleDepth > 0) angleDepth--;
        } else if (t == TT::COMMA && parenDepth == 1 && angleDepth == 0) {
            finishSlot(i);
            slotStart = i + 1;
        }
        i++;
    }
    return names;
}

// Resolve a constructor's parameter names by walking the token stream
// to the matching `class <className> { ... constructor(...) ... }`.
// SymbolRegistrationVisitor (the LSP-side AST walker) does not register
// constructors into ClassDefinition, so `findConstructor` would always
// return null here — fall back to the textual scan, picking the first
// constructor whose param count matches `argCount`. Multiple overloads
// are uncommon in mType source; v1 prefers any matching overload.
std::vector<std::string> lookupConstructorParams(
    const Document* doc, const std::string& className, std::size_t argCount)
{
    std::vector<std::string> out;
    if (!doc) return out;
    const auto& toks = doc->tokens;

    for (std::size_t i = 0; i + 2 < toks.size(); ++i) {
        if (toks[i].type != TT::CLASS) continue;
        if (toks[i + 1].type != TT::IDENTIFIER) continue;
        if (tokenText(doc, toks[i + 1]) != className) continue;

        // Find the class body opening brace.
        std::size_t j = i + 2;
        while (j < toks.size() && toks[j].type != TT::LBRACE) j++;
        if (j == toks.size()) return out;

        // Walk the class body, looking for `constructor(` at depth 1
        // (immediate body, not inside a nested method body).
        int braceDepth = 1;
        for (std::size_t k = j + 1; k < toks.size() && braceDepth > 0; ++k) {
            if (toks[k].type == TT::LBRACE) braceDepth++;
            else if (toks[k].type == TT::RBRACE) braceDepth--;
            else if (toks[k].type == TT::CONSTRUCTOR && braceDepth == 1
                     && k + 1 < toks.size()
                     && toks[k + 1].type == TT::LPAREN) {
                auto names = parseParamNamesFromTokenList(doc, toks, k + 1);
                if (names.size() == argCount) {
                    return names;
                }
                // Wrong overload — keep looking.
            }
        }
        return out;  // class found but no matching constructor
    }
    return out;
}

} // namespace

void InlayHintHandler::emitParameterHints(
    const Document* doc,
    const std::vector<CallSite>& calls,
    const ReceiverTypeMap& receiverTypes,
    std::vector<InlayHint>& out) const
{
    for (const auto& call : calls) {
        if (call.args.empty()) continue;
        if (isBuiltInName(call.calleeName)) continue;

        std::vector<std::string> paramNames;
        switch (call.kind) {
            case CalleeKind::Function:
                paramNames = lookupFunctionParams(doc, call.calleeName);
                break;
            case CalleeKind::Constructor:
                paramNames = lookupConstructorParams(doc, call.calleeName, call.args.size());
                break;
            case CalleeKind::Method: {
                // `this` falls through to "no hint" — the precomputed
                // map doesn't include it, and v1 doesn't resolve
                // `this.method(...)` to the enclosing class.
                // MYT-363 — chained links already have a pre-resolved
                // class from the prior link's return type; skip the
                // var-name lookup in that case.
                std::string className = call.receiverClassName;
                if (className.empty()) {
                    auto it = receiverTypes.find(call.receiverName);
                    if (it == receiverTypes.end() || it->second.empty()) continue;
                    className = it->second;
                }
                paramNames = lookupMethodParams(doc, className, call.calleeName);
                break;
            }
            case CalleeKind::StaticMethod:
                // receiverName IS the class name for `Class::method(...)`.
                paramNames = lookupStaticMethodParams(doc, call.receiverName, call.calleeName);
                break;
        }
        if (paramNames.empty()) continue;

        std::size_t n = std::min(paramNames.size(), call.args.size());
        for (std::size_t i = 0; i < n; ++i) {
            const std::string& pname = paramNames[i];
            if (pname.empty()) continue;
            const CallArg& arg = call.args[i];
            if (argumentDuplicatesParamName(arg, pname)) continue;

            InlayHint h;
            h.position = arg.startPos;
            h.label = pname + ":";
            h.kind = InlayHintKind::Parameter;
            h.paddingRight = true;
            out.push_back(std::move(h));
        }
    }
}

void InlayHintHandler::emitLambdaTypeHints(
    const Document* doc,
    const std::vector<LambdaSite>& lambdas,
    std::vector<InlayHint>& out) const
{
    if (!doc || !doc->environment) return;

    for (const auto& lam : lambdas) {
        if (lam.targetInterface.empty()) continue;
        auto iface = doc->environment->findInterface(lam.targetInterface);
        if (!iface) continue;
        const auto* sam = iface->getFunctionalMethod();
        if (!sam) continue;
        const auto& params = sam->parameters;
        if (params.size() != lam.params.size()) continue;

        for (std::size_t i = 0; i < lam.params.size(); ++i) {
            const auto& sigParam = params[i];
            if (!sigParam.second) continue;
            std::string typeName = sigParam.second->toString();
            if (typeName.empty() || typeName == "unknown") continue;

            InlayHint h;
            h.position = lam.params[i].endPos;
            h.label = ": " + typeName;
            h.kind = InlayHintKind::Type;
            out.push_back(std::move(h));
        }
    }
}

} // namespace mtype::lsp
