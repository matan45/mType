#include "CodeActionHandler.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/ImportUtils.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../mType/util/ImportScan.hpp"
#include <cctype>
#include <sstream>
#include <string>

namespace {

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

} // namespace

namespace mtype::lsp {

CodeActionHandler::CodeActionHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr), workspaceIndex_(std::move(workspaceIndex)) {}

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
    // fires whenever the cursor is on a class declaration line, even
    // when there's no diagnostic on the range.
    auto implementActions = getImplementInterfaceActions(uri, range.start.line);
    actions.insert(actions.end(), implementActions.begin(), implementActions.end());

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

    // Get the content of the current line
    std::string content = doc->content;
    std::istringstream stream(content);
    std::string currentLine;
    int currentLineNum = 0;

    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    if (currentLineNum != line) {
        return actions;
    }

    // Simple parsing: look for "class ClassName implements InterfaceName"
    size_t classPos = currentLine.find("class ");
    size_t implementsPos = currentLine.find("implements ");

    if (classPos == std::string::npos || implementsPos == std::string::npos) {
        return actions;
    }

    // Extract class name
    size_t classNameStart = classPos + 6; // "class ".length()
    size_t classNameEnd = currentLine.find_first_of(" \t{", classNameStart);
    if (classNameEnd == std::string::npos) classNameEnd = currentLine.length();
    std::string className = currentLine.substr(classNameStart, classNameEnd - classNameStart);

    // Extract interface name
    size_t interfaceNameStart = implementsPos + 11; // "implements ".length()
    size_t interfaceNameEnd = currentLine.find_first_of(" \t{", interfaceNameStart);
    if (interfaceNameEnd == std::string::npos) interfaceNameEnd = currentLine.length();
    std::string interfaceName = currentLine.substr(interfaceNameStart, interfaceNameEnd - interfaceNameStart);

    // Trim whitespace
    interfaceName.erase(interfaceName.find_last_not_of(" \t\r\n") + 1);

    // Get required methods from interface, collecting the unique type
    // names referenced in parameter / return positions. We use that
    // set below to attach `import` edits for any type that isn't a
    // built-in, isn't a generic parameter declared on the implementing
    // class, and isn't already in scope.
    std::unordered_set<std::string> referencedTypes;
    auto requiredMethods = getRequiredMethods(interfaceName, doc, &referencedTypes);

    if (requiredMethods.empty()) {
        return actions;
    }

    // Generate code action to implement all methods
    CodeAction action;
    action.title = "Implement interface methods";
    action.kind = "quickfix";

    // Build method stubs with @Override annotation
    std::stringstream stubsBuilder;
    for (const auto& methodStub : requiredMethods) {
        stubsBuilder << "    @Override\n";
        stubsBuilder << "    " << methodStub << " {\n";
        stubsBuilder << "        // TODO: Implement method\n";
        stubsBuilder << "    }\n\n";
    }

    // Find the closing brace of the class
    std::istringstream braceStream(doc->content);
    std::string docLine;
    int closingBraceLine = line + 1;
    int braceLineNum = 0;
    int braceDepth = 0;
    bool foundClassBody = false;

    // Start from the class declaration line
    while (std::getline(braceStream, docLine)) {
        if (braceLineNum == line) {
            // This is the class declaration line, start counting braces
            foundClassBody = true;
            // Count opening braces on this line
            for (char c : docLine) {
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
            }
        } else if (foundClassBody) {
            // Count braces on subsequent lines
            for (char c : docLine) {
                if (c == '{') braceDepth++;
                else if (c == '}') {
                    braceDepth--;
                    if (braceDepth == 0) {
                        // Found the closing brace of the class
                        closingBraceLine = braceLineNum;
                        break;
                    }
                }
            }
            if (braceDepth == 0) break;
        }
        braceLineNum++;
    }

    // Create text edit to insert methods before the closing brace
    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.range.start.line = closingBraceLine;
    textEdit.range.start.character = 0;
    textEdit.range.end = textEdit.range.start;
    textEdit.newText = stubsBuilder.str();

    edit.changes[uri].push_back(textEdit);

    // Auto-import any referenced types not already in scope. Mirrors
    // the missing-import quick fix's approach: skip self-references,
    // route through the workspace symbol index, and reuse
    // util::findImportInsertLine + buildImportInsertEdit so the
    // emitted import lines match the rest of the codebase byte for byte.
    if (workspaceIndex_ && !referencedTypes.empty()) {
        const auto genericParams = extractClassGenericParams(currentLine);
        auto classRegistry = doc->environment ? doc->environment->getClassRegistry() : nullptr;
        auto interfaceRegistry = doc->environment ? doc->environment->getInterfaceRegistry() : nullptr;

        // Workspace index is built off-thread at initialise; short-block
        // so an action fired before the scan finishes still sees results.
        workspaceIndex_->waitForReady(std::chrono::milliseconds(50));

        const std::string referencingPath = UriUtils::uriToFilePath(uri);
        const int insertLine = util::findImportInsertLine(doc->content);
        std::unordered_set<std::string> emittedSpellings;

        for (const auto& typeName : referencedTypes) {
            if (typeName == className) continue;
            if (typeName == interfaceName) continue;  // already in scope
            if (isBuiltinTypeName(typeName)) continue;
            if (genericParams.count(typeName) > 0) continue;
            if (classRegistry && classRegistry->hasClass(typeName)) continue;
            if (interfaceRegistry && interfaceRegistry->findInterface(typeName)) continue;

            auto matches = workspaceIndex_->findByName(typeName, /*maxResults=*/1);
            if (matches.empty()) continue;
            const auto& match = matches.front();

            const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
            if (symbolPath == referencingPath) continue;

            const std::string spelling =
                analysis::WorkspaceSymbolIndex::computeImportSpelling(
                    symbolPath, referencingPath);

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

std::vector<std::string> CodeActionHandler::getRequiredMethods(
    const std::string& interfaceName,
    const Document* doc,
    std::unordered_set<std::string>* outReferencedTypes
) {
    std::vector<std::string> methods;

    if (!doc->environment) {
        return methods;
    }

    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!interfaceRegistry) {
        return methods;
    }

    // Get interface definition
    auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
    if (!interfaceDef) {
        return methods;
    }

    // Get method signatures from interface
    const auto& signatures = interfaceDef->getMethodSignatures();

    for (const auto& sig : signatures) {
        // Build method signature string. mType uses C/Java-style param
        // syntax (`int index`, not `index: int`); interface-implemented
        // methods are conventionally `public`.
        std::stringstream methodBuilder;
        methodBuilder << "public function " << sig.name << "(";

        bool first = true;
        for (const auto& [paramName, paramType] : sig.parameters) {
            if (!first) methodBuilder << ", ";
            const std::string typeName = paramType->getName();
            methodBuilder << typeName << " " << paramName;
            if (outReferencedTypes) extractTypeNames(typeName, *outReferencedTypes);
            first = false;
        }

        const std::string returnTypeName = sig.returnType->getName();
        methodBuilder << "): " << returnTypeName;
        if (outReferencedTypes) extractTypeNames(returnTypeName, *outReferencedTypes);

        methods.push_back(methodBuilder.str());
    }

    return methods;
}

bool CodeActionHandler::classHasMethod(
    const std::string& className,
    const std::string& methodName,
    const Document* doc
) {
    if (!doc->environment) {
        return false;
    }

    auto classRegistry = doc->environment->getClassRegistry();
    if (!classRegistry) {
        return false;
    }

    auto classDef = classRegistry->findClass(className);
    if (!classDef) {
        return false;
    }

    return classDef->hasMethod(methodName);
}

} // namespace mtype::lsp
