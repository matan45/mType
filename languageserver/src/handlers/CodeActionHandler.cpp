#include "CodeActionHandler.hpp"
#include "AccessorCodegen.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/ImportUtils.hpp"
#include "../utils/ProjectConfigProvider.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/environment/registry/InterfaceDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"
#include "../../../mType/types/TypeSubstitutionService.hpp"
#include "../../../mType/util/ImportScan.hpp"
#include "../analysis/InterfaceHierarchyWalker.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

namespace fs = std::filesystem;

using mtype::lsp::Document;
using mtype::lsp::Position;
using runtimeTypes::klass::InterfaceDefinition;
using runtimeTypes::klass::MethodSignature;

// Compute the start/end character offsets of the identifier surrounding
// the given character position on a single source line. Returns
// {character, character} (zero-width) when there is no identifier
// boundary at that position. Used by the typo quick fix so the
// replacement edit covers the full misspelled token, not just the
// 1-character point span the diagnostic carries.
std::pair<int, int> wordRangeAt(const std::string& line, int character) {
    if (line.empty() || character < 0) {
        return { character, character };
    }
    auto isIdent = [](char c) {
        return (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_';
    };

    // If the character is past end-of-line or not on an identifier byte,
    // try the position to its left (the squiggle is often anchored at
    // the byte right after the identifier).
    int pivot = character;
    if (pivot >= static_cast<int>(line.size()) || !isIdent(line[pivot])) {
        if (pivot > 0 && isIdent(line[pivot - 1])) {
            pivot = pivot - 1;
        } else {
            return { character, character };
        }
    }

    int start = pivot;
    while (start > 0 && isIdent(line[start - 1])) {
        --start;
    }
    int end = pivot;
    while (end < static_cast<int>(line.size()) && isIdent(line[end])) {
        ++end;
    }
    return { start, end };
}

// Lowercase primitive type keywords that never need an import. The
// capitalized wrapper classes (Int / Float / Bool / String / Promise)
// are deliberately NOT in this list — they're real classes living in
// lib/primitives/*.mt and have to be imported like any other type.
// Treating them as builtins here would silently suppress their
// auto-import edits.
bool isBuiltinTypeName(const std::string& name) {
    static const std::unordered_set<std::string> kBuiltins = {
        "int", "float", "bool", "string", "void", "object"
    };
    return kBuiltins.count(name) > 0;
}

// Walk a type expression (e.g. "List<Foo>", "Bar[]", "HashMap<K, V>")
// and emit each base identifier into `out`. Generic args, array
// suffixes, and whitespace are stripped so "List<Foo[]>" yields
// {"List", "Foo"}. Empty / non-identifier tokens are ignored.
void extractTypeNames(const std::string& typeExpr,
                      std::unordered_set<std::string>& out)
{
    std::string token;
    auto flush = [&]() {
        if (token.empty()) return;
        // Strip a trailing "[]" array suffix.
        while (token.size() >= 2
               && token.compare(token.size() - 2, 2, "[]") == 0) {
            token.resize(token.size() - 2);
        }
        if (!token.empty()) {
            const char first = token.front();
            if (std::isalpha(static_cast<unsigned char>(first)) || first == '_') {
                out.insert(token);
            }
        }
        token.clear();
    };
    for (char c : typeExpr) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            token.push_back(c);
        } else if (c == '[' || c == ']') {
            // Keep [] attached so the flush can strip a trailing pair.
            token.push_back(c);
        } else {
            flush();
        }
    }
    flush();
}

// Parse generic type parameter names declared on the implementing
// class header line (e.g. "class MyList<T> implements List<T>" →
// {"T"}). Used so we don't try to import the class's own type
// parameters as if they were external symbols.
//
// Limitation: the close-angle is matched as the FIRST `>` after the
// opening `<`, so a nested-generic header like
// `class Foo<T extends Bar<X>> implements IFoo` would stop at the
// inner `>` and miss the outer parameter list. mType doesn't support
// that form today, so this is fine — but anyone extending the language
// to allow it should swap this scan for a depth-aware one.
std::unordered_set<std::string> extractClassGenericParams(
    const std::string& classDeclLine)
{
    std::unordered_set<std::string> params;
    const size_t classPos = classDeclLine.find("class ");
    if (classPos == std::string::npos) return params;
    const size_t open = classDeclLine.find('<', classPos);
    if (open == std::string::npos) return params;
    const size_t close = classDeclLine.find('>', open + 1);
    if (close == std::string::npos) return params;
    extractTypeNames(classDeclLine.substr(open + 1, close - open - 1), params);
    return params;
}

// Pick line `lineNumber` (0-based) out of a document's content.
// Returns an empty string if the line is out of range.
std::string getLine(const std::string& content, int lineNumber) {
    if (lineNumber < 0) return "";
    int currentLine = 0;
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t newline = content.find('\n', pos);
        const size_t lineEnd = (newline == std::string::npos) ? content.size() : newline;
        if (currentLine == lineNumber) {
            std::string line = content.substr(pos, lineEnd - pos);
            // Strip trailing CR for CRLF inputs.
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        if (newline == std::string::npos) break;
        pos = newline + 1;
        ++currentLine;
    }
    return "";
}

std::string trimTypeBaseName(const std::string& name) {
    const size_t generic = name.find('<');
    return generic == std::string::npos ? name : name.substr(0, generic);
}

std::string renderType(
    const types::UnifiedTypePtr& type,
    const std::unordered_map<std::string, std::string>& substitutions)
{
    if (!type) return "void";
    const std::string rendered = type->toString();
    auto it = substitutions.find(rendered);
    if (it != substitutions.end()) return it->second;
    return rendered;
}

std::string methodSignatureKey(
    const MethodSignature& sig,
    const std::unordered_map<std::string, std::string>& substitutions)
{
    std::stringstream key;
    key << sig.name << "(";
    for (size_t i = 0; i < sig.parameters.size(); ++i) {
        if (i > 0) key << ",";
        key << renderType(sig.parameters[i].second, substitutions);
    }
    key << ")";
    return key.str();
}

struct RequiredMethod {
    const MethodSignature* signature = nullptr;
    std::unordered_map<std::string, std::string> substitutions;
};

// Hierarchy traversal delegates to analysis::walkInterfaceHierarchy so the
// implement-interface-methods quick fix and DocumentManager's missing-
// methods diagnostic stay in lock-step. The walker substitutes parent
// generic args recursively (so `Child<T> extends Parent<List<T>>` viewed
// as `Child<int>` resolves to `Parent<List<int>>`), which matches the
// compiler-side InterfaceRegistrar; the previous local implementation
// only substituted at the leaf, dropping coverage on multi-level chains.
void collectInterfaceMethods(
    const std::string& interfaceName,
    const std::shared_ptr<runtimeTypes::klass::InterfaceRegistry>& registry,
    const ::types::TypeSubstitutionService& subst,
    std::vector<RequiredMethod>& out,
    std::unordered_set<std::string>& emitted)
{
    mtype::lsp::analysis::walkInterfaceHierarchy(interfaceName, registry, subst,
        [&](const runtimeTypes::klass::InterfaceDefinition& iface,
            const std::unordered_map<std::string, std::string>& substitutions) {
            for (const auto& sig : iface.getMethodSignatures()) {
                const std::string key = methodSignatureKey(sig, substitutions);
                if (emitted.insert(key).second) {
                    out.push_back(RequiredMethod{&sig, substitutions});
                }
            }
        });
}

std::vector<const ast::nodes::classes::ClassNode*> getTopLevelClasses(const Document* doc) {
    std::vector<const ast::nodes::classes::ClassNode*> classes;
    if (!doc) return classes;
    for (const auto& root : doc->ast) {
        auto* program = dynamic_cast<ast::nodes::statements::ProgramNode*>(root.get());
        if (!program) continue;
        for (const auto& statement : program->getStatements()) {
            auto* cls = dynamic_cast<ast::nodes::classes::ClassNode*>(statement.get());
            if (cls) classes.push_back(cls);
        }
    }
    return classes;
}

std::optional<Position> findClassClosingBrace(const std::string& content, int classLine) {
    std::istringstream stream(content);
    std::string line;
    int lineNumber = 0;
    int depth = 0;
    bool inClass = false;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (lineNumber < classLine) {
            ++lineNumber;
            continue;
        }

        for (int col = 0; col < static_cast<int>(line.size()); ++col) {
            if (line[col] == '{') {
                ++depth;
                inClass = true;
            } else if (line[col] == '}' && inClass) {
                --depth;
                if (depth == 0) {
                    return Position{lineNumber, col};
                }
            }
        }
        ++lineNumber;
    }

    return std::nullopt;
}

struct ClassTarget {
    const ast::nodes::classes::ClassNode* node = nullptr;
    int declarationLine = 0;
    Position insertionPoint{0, 0};
    std::string declarationText;
};

std::optional<ClassTarget> findClassTarget(
    const Document* doc,
    int line,
    bool declarationLineOnly)
{
    for (const auto* cls : getTopLevelClasses(doc)) {
        if (!cls) continue;
        const int declLine = cls->getLocation().getLine() - 1;
        auto close = findClassClosingBrace(doc->content, declLine);
        if (!close) continue;

        const bool matches = declarationLineOnly
            ? (declLine == line)
            : (declLine <= line && line <= close->line);
        if (!matches) continue;

        return ClassTarget{
            cls,
            declLine,
            *close,
            getLine(doc->content, declLine)
        };
    }
    return std::nullopt;
}

bool classHasFieldOnLine(const ast::nodes::classes::ClassNode* cls, int line)
{
    if (!cls) return false;
    for (const auto& node : cls->getFields()) {
        const auto* field = dynamic_cast<const ast::nodes::classes::FieldNode*>(node.get());
        if (!field) continue;
        if (field->getLocation().getLine() - 1 == line) {
            return true;
        }
    }
    return false;
}

std::unordered_set<std::string> collectClassGenericParams(
    const ast::nodes::classes::ClassNode* cls,
    const std::string& declarationLine)
{
    std::unordered_set<std::string> params = extractClassGenericParams(declarationLine);
    if (!cls) return params;
    for (const auto& param : cls->getGenericParameters()) {
        params.insert(param.name);
    }
    return params;
}

bool existingMethodMatches(
    const runtimeTypes::klass::MethodDefinition& method,
    const MethodSignature& sig,
    const std::unordered_map<std::string, std::string>& substitutions)
{
    if (method.getName() != sig.name) return false;
    if (method.getParameters().size() != sig.parameters.size()) return false;

    const auto& existingUnifiedParams = method.getUnifiedParameters();
    if (existingUnifiedParams.size() == sig.parameters.size()) {
        for (size_t i = 0; i < sig.parameters.size(); ++i) {
            if (!existingUnifiedParams[i].second) continue;
            const std::string expected = renderType(sig.parameters[i].second, substitutions);
            if (existingUnifiedParams[i].second->toString() != expected) {
                return false;
            }
        }
    }

    return true;
}

bool classHasMethod(
    const runtimeTypes::klass::ClassDefinition& cls,
    const RequiredMethod& required)
{
    if (!required.signature) return true;
    const auto overloads = cls.getAllInstanceMethodOverloads(required.signature->name);
    for (const auto& method : overloads) {
        if (method && existingMethodMatches(*method, *required.signature, required.substitutions)) {
            return true;
        }
    }
    return false;
}

std::string buildMethodStub(
    const RequiredMethod& required,
    std::unordered_set<std::string>& referencedTypes)
{
    const auto& sig = *required.signature;
    std::stringstream methodBuilder;
    methodBuilder << "public function " << sig.name << "(";

    for (size_t i = 0; i < sig.parameters.size(); ++i) {
        if (i > 0) methodBuilder << ", ";
        const std::string typeName = renderType(sig.parameters[i].second, required.substitutions);
        methodBuilder << typeName << " " << sig.parameters[i].first;
        extractTypeNames(typeName, referencedTypes);
    }

    const std::string returnTypeName = renderType(sig.returnType, required.substitutions);
    methodBuilder << "): " << returnTypeName;
    extractTypeNames(returnTypeName, referencedTypes);

    return methodBuilder.str();
}

std::string aliasImportSpelling(
    const mtype::lsp::ProjectConfigProvider& config,
    const std::string& symbolFilePath)
{
    std::error_code ec;
    const fs::path symbolPath = fs::weakly_canonical(fs::path(symbolFilePath), ec);
    if (ec) return "";

    for (const auto& [alias, aliasPathText] : config.getAliases()) {
        std::error_code aliasEc;
        const fs::path aliasPath = fs::weakly_canonical(fs::path(aliasPathText), aliasEc);
        if (aliasEc) continue;

        std::error_code relEc;
        fs::path rel = fs::relative(symbolPath, aliasPath, relEc);
        if (relEc || rel.empty()) continue;
        const std::string relText = rel.generic_string();
        if (relText.rfind("..", 0) == 0) continue;

        std::string spelling = alias + "/" + relText;
        if (spelling.size() < 3
            || spelling.compare(spelling.size() - 3, 3, ".mt") != 0) {
            spelling += ".mt";
        }
        return spelling;
    }

    return "";
}

} // namespace

namespace mtype::lsp {

CodeActionHandler::CodeActionHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr), workspaceIndex_(std::move(workspaceIndex)) {}

void CodeActionHandler::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config) {
    projectConfig_ = std::move(config);
}

std::vector<CodeAction> CodeActionHandler::handleCodeAction(
    const std::string& uri,
    const Range& range,
    const std::vector<Diagnostic>& diagnostics
) {
    std::vector<CodeAction> actions;

    // MYT-35 Phase 5 — diagnostic-driven dispatch. For each diagnostic
    // attached to the cursor's range, look at its `code`/`data` and
    // emit any fixes that apply.
    for (const auto& diag : diagnostics) {
        auto fixes = generateFixesForDiagnostic(uri, diag);
        actions.insert(actions.end(), fixes.begin(), fixes.end());
    }

    // Diagnostic-agnostic refactor: implement interface methods. This
    // fires when the cursor is inside a class with missing interface
    // methods; diagnostic-triggered calls use the same path.
    const bool alreadyHasImplementAction =
        std::any_of(actions.begin(), actions.end(), [](const CodeAction& action) {
            return action.title.find("Implement interface methods") != std::string::npos;
        });
    if (!alreadyHasImplementAction) {
        auto implementActions = getImplementInterfaceActions(uri, range.start.line);
        actions.insert(actions.end(), implementActions.begin(), implementActions.end());
    }

    // Diagnostic-agnostic refactors: scaffold field accessors and a
    // default constructor when the cursor is inside a class.
    auto getterSetterActions = generateGetterSetterActions(uri, range.start.line);
    actions.insert(actions.end(), getterSetterActions.begin(), getterSetterActions.end());

    auto constructorActions = generateDefaultConstructorAction(uri, range.start.line);
    actions.insert(actions.end(), constructorActions.begin(), constructorActions.end());

    return actions;
}

std::vector<CodeAction> CodeActionHandler::generateFixesForDiagnostic(
    const std::string& uri,
    const Diagnostic& diagnostic
) {
    std::vector<CodeAction> actions;

    // MYT-47 — missing-import quick fix. Diagnostics produced by
    // ClassNotFoundException / UndefinedException carry their source
    // exception type in `data.exceptionType`. Try the workspace
    // symbol index first; the typo fix below remains as a fallback.
    if (diagnostic.data
        && diagnostic.data->is_object()
        && diagnostic.data->contains("exceptionType"))
    {
        const auto& exType = (*diagnostic.data)["exceptionType"];
        if (exType.is_string())
        {
            const std::string s = exType.get<std::string>();
            if (s == "ClassNotFoundException" || s == "UndefinedException")
            {
                auto importFixes = generateMissingImportFixes(uri, diagnostic);
                actions.insert(actions.end(), importFixes.begin(), importFixes.end());
            }
            else if (s == "PrimitiveInGenericException")
            {
                // MYT-360 — replace `<int>` with `<Int>` and auto-import.
                auto primFixes = generatePrimitiveInGenericFixes(uri, diagnostic);
                actions.insert(actions.end(), primFixes.begin(), primFixes.end());
            }
        }
    }

    // The Phase 4 ExceptionConverter attaches a `data["suggestions"]`
    // array whenever did-you-mean produced a candidate. Any diagnostic
    // that has at least one suggestion can drive a typo fix, regardless
    // of which Name* code id triggered it (E1001 variable, E1002 function,
    // E1004 method, E1005 field, E1003/E1006 class). The dispatch is
    // therefore driven by the data shape, not the code id.
    if (diagnostic.data
        && diagnostic.data->is_object()
        && diagnostic.data->contains("suggestions")
        && (*diagnostic.data)["suggestions"].is_array()
        && !(*diagnostic.data)["suggestions"].empty())
    {
        auto typoFixes = generateTypoFixActions(uri, diagnostic);
        actions.insert(actions.end(), typoFixes.begin(), typoFixes.end());
    }

    auto implementFixes = getImplementInterfaceActions(
        uri, diagnostic.range.start.line);
    for (auto& action : implementFixes) {
        action.diagnostics.push_back(diagnostic);
        actions.push_back(std::move(action));
    }

    return actions;
}

std::vector<CodeAction> CodeActionHandler::generateMissingImportFixes(
    const std::string& uri,
    const Diagnostic& diagnostic
) {
    std::vector<CodeAction> actions;
    if (!workspaceIndex_) return actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) return actions;

    // Extract the missing identifier from the source line at the
    // diagnostic's anchor. Reuses the same word-extraction helpers as
    // the typo fix.
    const std::string line = getLine(doc->content, diagnostic.range.start.line);
    const auto [wordStart, wordEnd] =
        wordRangeAt(line, diagnostic.range.start.character);
    if (wordEnd <= wordStart) return actions;
    const std::string identifier = line.substr(wordStart, wordEnd - wordStart);
    if (identifier.empty()) return actions;

    // Short-block until the initial workspace scan is populated. Without
    // this, an early code action that fires before the async scan
    // finishes would see an empty index and offer no quick fix.
    workspaceIndex_->waitForReady(std::chrono::milliseconds(50));

    auto matches = workspaceIndex_->findByName(identifier, /*maxResults=*/5);
    if (matches.empty()) return actions;

    // MYT-51 — insertion-line scan extracted to util::findImportInsertLine
    // so the auto-import completion path produces byte-identical edits.
    const int insertLine = util::findImportInsertLine(doc->content);
    const std::string referencingPath = UriUtils::uriToFilePath(uri);

    for (const auto& match : matches)
    {
        const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
        if (symbolPath == referencingPath)
        {
            continue; // self-import would be nonsense
        }

        const std::string spelling =
            analysis::WorkspaceSymbolIndex::computeImportSpelling(
                symbolPath, referencingPath);

        CodeAction action;
        action.title = "Add import '" + identifier + "' from \"" + spelling + "\"";
        action.kind = "quickfix";
        action.diagnostics.push_back(diagnostic);

        WorkspaceEdit edit;
        edit.changes[uri].push_back(
            utils::buildImportInsertEdit(insertLine, identifier, spelling));
        action.edit = edit;

        actions.push_back(std::move(action));
    }

    return actions;
}

std::vector<CodeAction> CodeActionHandler::generatePrimitiveInGenericFixes(
    const std::string& uri,
    const Diagnostic& diagnostic
) {
    std::vector<CodeAction> actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) return actions;

    // Parse `primitive` and `boxed` out of the canonical message shape:
    // "Primitive type 'int' cannot be used as generic type argument.
    //  Use boxed type 'Int' instead". The exception type already gated us
    // here, so a missed match means the message format changed under us —
    // bail rather than guess.
    const std::string& msg = diagnostic.message;
    const size_t q1 = msg.find('\'');
    if (q1 == std::string::npos) return actions;
    const size_t q2 = msg.find('\'', q1 + 1);
    if (q2 == std::string::npos) return actions;
    const size_t q3 = msg.find('\'', q2 + 1);
    if (q3 == std::string::npos) return actions;
    const size_t q4 = msg.find('\'', q3 + 1);
    if (q4 == std::string::npos) return actions;

    const std::string primitive = msg.substr(q1 + 1, q2 - q1 - 1);
    const std::string boxed = msg.substr(q3 + 1, q4 - q3 - 1);
    if (primitive.empty() || boxed.empty()) return actions;

    // Locate the primitive token in the source. The diagnostic range is
    // anchored at the parser's stream location when it threw — typically
    // on or just after the primitive token, but with a 1-character span.
    // Search the diagnostic's line for the primitive as a whole word and
    // pick the occurrence whose start is closest to the diagnostic anchor.
    const int anchorLine = diagnostic.range.start.line;
    const int anchorCol = diagnostic.range.start.character;
    const std::string line = getLine(doc->content, anchorLine);
    if (line.empty()) return actions;

    auto isIdent = [](char c) {
        return (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_';
    };

    int bestStart = -1;
    int bestDistance = std::numeric_limits<int>::max();
    size_t searchFrom = 0;
    while (searchFrom <= line.size()) {
        const size_t hit = line.find(primitive, searchFrom);
        if (hit == std::string::npos) break;
        const size_t hitEnd = hit + primitive.size();
        const bool leftOk = (hit == 0) || !isIdent(line[hit - 1]);
        const bool rightOk = (hitEnd >= line.size()) || !isIdent(line[hitEnd]);
        if (leftOk && rightOk) {
            const int distance = std::abs(static_cast<int>(hit) - anchorCol);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestStart = static_cast<int>(hit);
            }
        }
        searchFrom = hit + 1;
    }
    if (bestStart < 0) return actions;

    CodeAction action;
    action.title = "Replace '" + primitive + "' with '" + boxed
                   + "' and add import";
    action.kind = "quickfix";
    action.diagnostics.push_back(diagnostic);

    WorkspaceEdit edit;

    // 1. Replace the primitive token with the boxed type name.
    TextEdit replaceEdit;
    replaceEdit.range.start.line = anchorLine;
    replaceEdit.range.start.character = bestStart;
    replaceEdit.range.end.line = anchorLine;
    replaceEdit.range.end.character = bestStart + static_cast<int>(primitive.size());
    replaceEdit.newText = boxed;
    edit.changes[uri].push_back(std::move(replaceEdit));

    // 2. Auto-import the boxed type when it isn't already visible. Same
    // "already-imported?" check the auto-import completion path uses
    // (AutoImportCompletionProvider::enrichWithWrapperImport).
    auto classRegistry = doc->environment
        ? doc->environment->getClassRegistry()
        : nullptr;
    const bool alreadyImported =
        classRegistry && classRegistry->hasClass(boxed);

    if (!alreadyImported && workspaceIndex_) {
        workspaceIndex_->waitForReady(std::chrono::milliseconds(50));
        auto matches = workspaceIndex_->findByName(boxed, /*maxResults=*/1);
        if (!matches.empty()) {
            const std::string referencingPath = UriUtils::uriToFilePath(uri);
            const std::string symbolPath =
                UriUtils::uriToFilePath(matches.front().fileUri);
            if (symbolPath != referencingPath) {
                const std::string spelling =
                    analysis::WorkspaceSymbolIndex::computeImportSpelling(
                        symbolPath, referencingPath);
                const int insertLine = util::findImportInsertLine(doc->content);
                edit.changes[uri].push_back(
                    utils::buildImportInsertEdit(insertLine, boxed, spelling));
            }
        }
    }

    action.edit = edit;
    actions.push_back(std::move(action));
    return actions;
}

std::vector<CodeAction> CodeActionHandler::generateTypoFixActions(
    const std::string& uri,
    const Diagnostic& diagnostic
) {
    std::vector<CodeAction> actions;

    if (!diagnostic.data || !diagnostic.data->is_object()) {
        return actions;
    }

    const auto& data = *diagnostic.data;
    if (!data.contains("suggestions") || !data["suggestions"].is_array()) {
        return actions;
    }

    // Compute the actual identifier range under the diagnostic's
    // anchor position. The Phase 3 LSP converter draws a 1-character
    // point span for diagnostics whose source location has no end —
    // good enough to put a squiggle on, but the typo replacement
    // needs to cover the full misspelled token.
    Range editRange = diagnostic.range;
    if (auto doc = documentManager_->getDocument(uri)) {
        const std::string line = getLine(doc->content, diagnostic.range.start.line);
        const auto [wordStart, wordEnd] =
            wordRangeAt(line, diagnostic.range.start.character);
        if (wordEnd > wordStart) {
            editRange.start.line = diagnostic.range.start.line;
            editRange.start.character = wordStart;
            editRange.end.line = diagnostic.range.start.line;
            editRange.end.character = wordEnd;
        }
    }

    // Each suggestion in the data blob carries a `label` like
    // "did you mean 'foo'?" (set by ExceptionConverter::attachDidYouMean).
    // Pull the candidate identifier out of the label so we can use it
    // both as the action title and as the replacement text.
    for (const auto& suggestionJson : data["suggestions"]) {
        if (!suggestionJson.is_object()) {
            continue;
        }

        // Structured-edits path. When the diagnostic source attached
        // explicit edits (e.g. ExceptionConverter::convertMissingSemicolon
        // emits a zero-width "insert ';'" edit), use those verbatim
        // instead of synthesising a range via wordRangeAt — the producer
        // knows the exact range and the synthesis would expand a `;`
        // insert into a replace over the next identifier (MYT-364).
        //
        // The presence of the `edits` key (even when empty) signals that
        // the producer chose the structured route deliberately, so we
        // skip the legacy label-based synthesis for this suggestion
        // either way: producing the right action when edits are valid,
        // producing no action when they're missing/malformed. Falling
        // through to the typo path would re-introduce the MYT-364
        // overwrite for labels like "insert ';'" whose extracted
        // candidate (`;`) feeds wordRangeAt incorrectly.
        if (suggestionJson.contains("edits")) {
            if (!suggestionJson["edits"].is_array()
                || suggestionJson["edits"].empty())
            {
                continue;
            }
            WorkspaceEdit edit;
            for (const auto& editJson : suggestionJson["edits"]) {
                if (!editJson.is_object()
                    || !editJson.contains("start")
                    || !editJson.contains("end")
                    || !editJson.contains("newText")
                    || !editJson["start"].is_object()
                    || !editJson["end"].is_object()
                    || !editJson["newText"].is_string())
                {
                    continue;
                }
                TextEdit textEdit;
                textEdit.range.start.line =
                    editJson["start"].value("line", 0);
                textEdit.range.start.character =
                    editJson["start"].value("character", 0);
                textEdit.range.end.line =
                    editJson["end"].value("line", 0);
                textEdit.range.end.character =
                    editJson["end"].value("character", 0);
                textEdit.newText = editJson["newText"].get<std::string>();
                edit.changes[uri].push_back(std::move(textEdit));
            }
            // If every entry was malformed, skip to avoid emitting a
            // no-op action — the legacy label path below cannot help
            // either since the producer signalled it wanted explicit
            // edits.
            if (edit.changes[uri].empty()) {
                continue;
            }

            CodeAction action;
            // Title from the label if present (capitalize the first
            // letter so "insert ';'" → "Insert ';'"); otherwise use the
            // newText of the first edit as a last-resort label.
            std::string title;
            if (suggestionJson.contains("label")
                && suggestionJson["label"].is_string())
            {
                title = suggestionJson["label"].get<std::string>();
                if (!title.empty()) {
                    title[0] = static_cast<char>(
                        std::toupper(static_cast<unsigned char>(title[0])));
                }
            }
            if (title.empty()) {
                title = "Quick fix: "
                    + edit.changes[uri].front().newText;
            }
            action.title = std::move(title);
            action.kind = "quickfix";
            action.diagnostics.push_back(diagnostic);
            action.edit = std::move(edit);
            actions.push_back(std::move(action));
            continue;
        }

        if (!suggestionJson.contains("label")) {
            continue;
        }
        const std::string label = suggestionJson["label"].get<std::string>();
        // Extract the candidate name from inside the single quotes.
        const size_t quoteStart = label.find('\'');
        if (quoteStart == std::string::npos) {
            continue;
        }
        const size_t quoteEnd = label.find('\'', quoteStart + 1);
        if (quoteEnd == std::string::npos) {
            continue;
        }
        const std::string candidate =
            label.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        if (candidate.empty()) {
            continue;
        }

        CodeAction action;
        action.title = "Replace with '" + candidate + "'";
        action.kind = "quickfix";
        action.diagnostics.push_back(diagnostic);

        WorkspaceEdit edit;
        TextEdit textEdit;
        textEdit.range = editRange;
        textEdit.newText = candidate;
        edit.changes[uri].push_back(textEdit);
        action.edit = edit;

        actions.push_back(std::move(action));
    }

    return actions;
}

std::vector<CodeAction> CodeActionHandler::getImplementInterfaceActions(
    const std::string& uri,
    int line
) {
    std::vector<CodeAction> actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return actions;
    }

    auto target = findClassTarget(doc, line, /*declarationLineOnly=*/false);
    if (!target || !target->node) {
        return actions;
    }

    const auto& implementedInterfaces = target->node->getImplementedInterfaces();
    if (implementedInterfaces.empty()) {
        return actions;
    }

    auto classRegistry = doc->environment->getClassRegistry();
    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!classRegistry || !interfaceRegistry) {
        return actions;
    }

    auto classDef = classRegistry->findClass(target->node->getClassName());
    if (!classDef) {
        return actions;
    }

    std::vector<RequiredMethod> requiredMethods;
    std::unordered_set<std::string> emittedMethodKeys;
    ::types::TypeSubstitutionService subst;
    for (const auto& interfaceName : implementedInterfaces) {
        collectInterfaceMethods(
            interfaceName,
            interfaceRegistry,
            subst,
            requiredMethods,
            emittedMethodKeys);
    }

    std::vector<RequiredMethod> missingMethods;
    for (const auto& required : requiredMethods) {
        if (!required.signature) continue;
        if (!classHasMethod(*classDef, required)) {
            missingMethods.push_back(required);
        }
    }

    if (missingMethods.empty()) {
        return actions;
    }

    CodeAction action;
    action.title = "Implement interface methods";
    action.kind = "quickfix";

    std::unordered_set<std::string> referencedTypes;
    std::stringstream stubsBuilder;
    for (const auto& required : missingMethods) {
        const std::string methodStub = buildMethodStub(required, referencedTypes);
        stubsBuilder << "    @Override\n";
        stubsBuilder << "    " << methodStub << " {\n";
        stubsBuilder << "        // TODO: Implement method\n";
        stubsBuilder << "    }\n\n";
    }

    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.range.start = target->insertionPoint;
    textEdit.range.end = textEdit.range.start;
    textEdit.newText = target->insertionPoint.character == 0
        ? stubsBuilder.str()
        : "\n" + stubsBuilder.str();

    edit.changes[uri].push_back(textEdit);

    // Auto-import any referenced types not already in scope. Mirrors
    // the missing-import quick fix's approach: skip self-references,
    // route through the workspace symbol index, and reuse
    // util::findImportInsertLine + buildImportInsertEdit so the
    // emitted import lines match the rest of the codebase byte for byte.
    if (workspaceIndex_ && !referencedTypes.empty()) {
        const auto genericParams = collectClassGenericParams(
            target->node, target->declarationText);
        std::unordered_set<std::string> implementedInterfaceNames;
        for (const auto& interfaceName : implementedInterfaces) {
            implementedInterfaceNames.insert(trimTypeBaseName(interfaceName));
        }

        // Workspace index is built off-thread at initialise; short-block
        // so an action fired before the scan finishes still sees results.
        workspaceIndex_->waitForReady(std::chrono::milliseconds(50));

        const std::string referencingPath = UriUtils::uriToFilePath(uri);
        const int insertLine = util::findImportInsertLine(doc->content);
        std::unordered_set<std::string> emittedSpellings;

        for (const auto& typeName : referencedTypes) {
            if (typeName == target->node->getClassName()) continue;
            if (implementedInterfaceNames.count(typeName) > 0) continue;
            if (isBuiltinTypeName(typeName)) continue;
            if (genericParams.count(typeName) > 0) continue;
            if (classRegistry && classRegistry->hasClass(typeName)) continue;
            if (interfaceRegistry && interfaceRegistry->findInterface(typeName)) continue;

            auto matches = workspaceIndex_->findByName(typeName, /*maxResults=*/1);
            if (matches.empty()) continue;
            const auto& match = matches.front();

            const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
            if (symbolPath == referencingPath) continue;

            std::string spelling;
            if (projectConfig_ && projectConfig_->isLoaded()) {
                spelling = aliasImportSpelling(*projectConfig_, symbolPath);
            }
            if (spelling.empty()) {
                spelling = analysis::WorkspaceSymbolIndex::computeImportSpelling(
                    symbolPath, referencingPath);
            }

            const std::string dedupKey = typeName + "\n" + spelling;
            if (!emittedSpellings.insert(dedupKey).second) continue;

            edit.changes[uri].push_back(
                utils::buildImportInsertEdit(insertLine, typeName, spelling));
        }
    }

    action.edit = edit;

    actions.push_back(action);

    return actions;
}

std::vector<CodeAction> CodeActionHandler::generateGetterSetterActions(
    const std::string& uri,
    int line
) {
    std::vector<CodeAction> actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return actions;
    }

    auto target = findClassTarget(doc, line, /*declarationLineOnly=*/false);
    if (!target || !target->node) {
        return actions;
    }
    if (!classHasFieldOnLine(target->node, line)) {
        return actions;
    }

    const std::string body = accessorgen::buildAccessorsBody(*target->node);
    if (body.empty()) {
        return actions;  // every eligible field already has its accessors
    }

    CodeAction action;
    action.title = "Generate getters and setters for all fields";
    action.kind = "refactor";

    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.range.start = target->insertionPoint;
    textEdit.range.end = textEdit.range.start;
    textEdit.newText = target->insertionPoint.character == 0
        ? body
        : "\n" + body;
    edit.changes[uri].push_back(textEdit);
    action.edit = edit;

    actions.push_back(std::move(action));
    return actions;
}

std::vector<CodeAction> CodeActionHandler::generateDefaultConstructorAction(
    const std::string& uri,
    int line
) {
    std::vector<CodeAction> actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return actions;
    }

    auto target = findClassTarget(doc, line, /*declarationLineOnly=*/false);
    if (!target || !target->node) {
        return actions;
    }

    const std::string body = accessorgen::buildDefaultConstructorBody(*target->node);
    if (body.empty()) {
        return actions;  // a zero-arg constructor already exists
    }

    CodeAction action;
    action.title = "Generate default constructor";
    action.kind = "refactor";

    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.range.start = target->insertionPoint;
    textEdit.range.end = textEdit.range.start;
    textEdit.newText = target->insertionPoint.character == 0
        ? body
        : "\n" + body;
    edit.changes[uri].push_back(textEdit);
    action.edit = edit;

    actions.push_back(std::move(action));
    return actions;
}

} // namespace mtype::lsp
