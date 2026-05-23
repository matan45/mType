#include "CodeActionHandler.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/ImportUtils.hpp"
#include "../utils/ProjectConfigProvider.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/environment/registry/InterfaceDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"
#include "../../../mType/util/ImportScan.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
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

std::vector<std::string> splitGenericArguments(const std::string& typeName) {
    std::vector<std::string> args;
    const size_t open = typeName.find('<');
    const size_t close = typeName.rfind('>');
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return args;
    }

    int depth = 0;
    std::string current;
    const std::string inner = typeName.substr(open + 1, close - open - 1);
    for (char c : inner) {
        if (c == '<') {
            ++depth;
            current.push_back(c);
        } else if (c == '>') {
            --depth;
            current.push_back(c);
        } else if (c == ',' && depth == 0) {
            size_t start = current.find_first_not_of(" \t\r\n");
            size_t end = current.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                args.push_back(current.substr(start, end - start + 1));
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }

    size_t start = current.find_first_not_of(" \t\r\n");
    size_t end = current.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) {
        args.push_back(current.substr(start, end - start + 1));
    }
    return args;
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

void collectInterfaceMethods(
    const std::string& interfaceName,
    const std::shared_ptr<runtimeTypes::klass::InterfaceRegistry>& registry,
    std::vector<RequiredMethod>& out,
    std::unordered_set<std::string>& emitted,
    std::unordered_set<std::string>& visiting)
{
    if (!registry) return;

    const std::string baseName = trimTypeBaseName(interfaceName);
    if (!visiting.insert(baseName).second) return;

    auto iface = registry->findInterface(interfaceName);
    if (!iface) {
        visiting.erase(baseName);
        return;
    }

    std::unordered_map<std::string, std::string> substitutions;
    const auto args = splitGenericArguments(interfaceName);
    const auto& genericParams = iface->getGenericParameters();
    for (size_t i = 0; i < args.size() && i < genericParams.size(); ++i) {
        substitutions[genericParams[i].name] = args[i];
    }

    for (const auto& parent : iface->getExtendedInterfaces()) {
        collectInterfaceMethods(parent, registry, out, emitted, visiting);
    }

    for (const auto& sig : iface->getMethodSignatures()) {
        const std::string key = methodSignatureKey(sig, substitutions);
        if (emitted.insert(key).second) {
            out.push_back(RequiredMethod{&sig, substitutions});
        }
    }

    visiting.erase(baseName);
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
        if (!suggestionJson.is_object() || !suggestionJson.contains("label")) {
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
    std::unordered_set<std::string> visitingInterfaces;
    for (const auto& interfaceName : implementedInterfaces) {
        collectInterfaceMethods(
            interfaceName,
            interfaceRegistry,
            requiredMethods,
            emittedMethodKeys,
            visitingInterfaces);
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

} // namespace mtype::lsp
