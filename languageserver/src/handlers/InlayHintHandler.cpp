#include "InlayHintHandler.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_set>

#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/environment/registry/FunctionRegistry.hpp"
#include "../../../mType/runtimeTypes/global/FunctionDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../mType/token/TokenType.hpp"
#include "../../../mType/types/UnifiedType.hpp"

namespace mtype::lsp {

namespace {

using TT = token::TokenType;

// Token::stringValue is a string_view into the lexer's source buffer,
// which DocumentManager destroys after parseDocument. Read the lexeme
// out of doc->content using (line, column, size) instead — the same
// dance RenameHandler does.
std::string tokenText(const Document* doc, const token::Token& tok) {
    if (!doc) return {};
    int line = tok.location.getLine() - 1;
    int col = tok.location.getColumn() - 1;
    std::size_t len = tok.stringValue.size();
    if (line < 0 || col < 0 || len == 0) return {};

    const std::string& content = doc->content;
    std::size_t pos = 0;
    int currentLine = 0;
    while (currentLine < line && pos < content.size()) {
        if (content[pos] == '\n') currentLine++;
        pos++;
    }
    if (currentLine != line) return {};
    std::size_t start = pos + static_cast<std::size_t>(col);
    if (start + len > content.size()) return {};
    return content.substr(start, len);
}

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
    p.character += static_cast<int>(tok.stringValue.size());
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
        collectSites(doc, range, calls, lambdas);
        emitParameterHints(doc, calls, out);
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
    std::vector<LambdaSite>& lambdas) const
{
    const auto& toks = doc->tokens;

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
                    // first token of the param list. We expect to find
                    //     <InterfaceName> <varName> = <param-list> -> ...
                    // immediately. Hitting any walk-back boundary
                    // (semicolon, paren, brace, bracket, comma) before
                    // ASSIGN means this lambda isn't a plain variable
                    // initializer — leave targetInterface empty.
                    std::size_t w = paramListStart;
                    bool foundAssign = false;
                    while (w > 0) {
                        w--;
                        if (toks[w].type == TT::ASSIGN) { foundAssign = true; break; }
                        if (isWalkBackBoundary(toks[w].type)) break;
                    }
                    if (foundAssign && w >= 2
                        && toks[w - 1].type == TT::IDENTIFIER
                        && toks[w - 2].type == TT::IDENTIFIER) {
                        lam.targetInterface = tokenText(doc, toks[w - 2]);
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
                    } else if (i >= 2 && toks[i - 2].type == TT::DOT) {
                        // Chained / complex receiver — v1 can't resolve.
                        f.isCall = false;
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
std::vector<std::string> lookupMethodParams(
    const Document* doc, const std::string& className,
    const std::string& methodName)
{
    std::vector<std::string> out;
    if (!doc || !doc->environment) return out;
    auto creg = doc->environment->getClassRegistry();
    if (!creg) return out;
    auto cls = creg->findClass(className);
    if (!cls) return out;
    auto method = cls->getMethod(methodName);
    if (!method) return out;
    const auto& params = method->getParameters();
    bool first = true;
    for (const auto& [pname, _ptype] : params) {
        if (first && pname == "this") { first = false; continue; }
        first = false;
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

// Resolve a variable's declared class from the token stream by finding
// `<ClassName> <varName>` near the call. The full Environment lookup
// would respect scope but v1 prefers the simpler textual rule that
// SignatureHelpHandler already uses for the same purpose.
std::string resolveReceiverClassName(
    const Document* doc, const std::string& receiverName, int /*callLine*/)
{
    if (!doc) return {};
    // `this` is a special receiver — resolve to the enclosing class via
    // any method declaration above the call. For simplicity, v1 only
    // handles plain object receivers; `this.method(...)` falls through.
    if (receiverName == "this") return {};

    // Walk tokens and look for a declaration `IDENTIFIER receiverName`
    // pattern. Take the FIRST one we find — matches the visible scope
    // in straight-line code.
    const auto& toks = doc->tokens;
    for (std::size_t i = 1; i < toks.size(); ++i) {
        const auto& cur = toks[i];
        if (cur.type != TT::IDENTIFIER) continue;
        if (cur.stringValue.size() != receiverName.size()) continue;
        if (tokenText(doc, cur) != receiverName) continue;
        const auto& prev = toks[i - 1];
        if (prev.type == TT::IDENTIFIER) {
            return tokenText(doc, prev);
        }
    }
    return {};
}

} // namespace

void InlayHintHandler::emitParameterHints(
    const Document* doc,
    const std::vector<CallSite>& calls,
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
                std::string cls = resolveReceiverClassName(doc, call.receiverName, call.callLine);
                if (cls.empty()) continue;
                paramNames = lookupMethodParams(doc, cls, call.calleeName);
                break;
            }
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
            h.paddingLeft = true;
            out.push_back(std::move(h));
        }
    }
}

} // namespace mtype::lsp
