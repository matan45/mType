#include "DocumentManager.hpp"
#include "../../mType/token/TokenType.hpp"
#include "../../mType/environment/EnvironmentBuilder.hpp"
#include "../../mType/environment/registry/ClassRegistry.hpp"
#include "../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../mType/ast/AccessModifier.hpp"
#include "../../mType/diagnostics/ExceptionConverter.hpp"
#include "../../mType/diagnostics/SourceFileCache.hpp"
#include "../../mType/analysis/OverrideAnnotationChecker.hpp"
#include "../../mType/analysis/UnusedVariableAnalyzer.hpp"
#include "utils/MemoryFileReader.hpp"
#include "analysis/SymbolRegistrationVisitor.hpp"
#include "analysis/ImportResolver.hpp"
#include "utils/ProjectConfigProvider.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

using namespace lexer;
using namespace token;
using namespace parser;
using namespace environment;
using namespace services;

namespace mtype::lsp {

DocumentManager::DocumentManager() {
    // ImportManager is now per-document for LSP
    importResolver_ = std::make_unique<ImportResolver>();
}

DocumentManager::~DocumentManager() = default;

void DocumentManager::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config) {
    if (importResolver_) {
        importResolver_->setProjectConfig(std::move(config));
    }
}

void DocumentManager::openDocument(const std::string& uri, const std::string& content, int version) {
    auto doc = std::make_unique<Document>();
    doc->uri = uri;
    doc->content = content;
    doc->version = version;

    documents_[uri] = std::move(doc);
    parseDocument(uri);
}

void DocumentManager::updateDocument(const std::string& uri, const std::string& content, int version) {
    auto it = documents_.find(uri);
    if (it != documents_.end()) {
        it->second->content = content;
        it->second->version = version;
        it->second->isParsed = false;
        parseDocument(uri);
    }
}

void DocumentManager::closeDocument(const std::string& uri) {
    // Drop the file's cached source so a stale snippet can't show up if
    // the same URI is reopened with different content.
    diagnostics::SourceFileCache::instance().invalidate(uri);
    documents_.erase(uri);
}

Document* DocumentManager::getDocument(const std::string& uri) {
    auto it = documents_.find(uri);
    return it != documents_.end() ? it->second.get() : nullptr;
}

const Document* DocumentManager::getDocument(const std::string& uri) const {
    auto it = documents_.find(uri);
    return it != documents_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> DocumentManager::getAllOpenUris() const {
    std::vector<std::string> uris;
    uris.reserve(documents_.size());
    for (const auto& [uri, _] : documents_) {
        uris.push_back(uri);
    }
    return uris;
}

void DocumentManager::parseDocument(const std::string& uri) {
    auto doc = getDocument(uri);
    if (!doc) {
        return;
    }

    // Publish the in-memory editor buffer to the diagnostic source cache
    // BEFORE parsing, so any throw from the parser/lexer can resolve its
    // span back to the unsaved content the user is currently editing.
    // The Lexer constructor also publishes (from disk-on-load), but the
    // LSP needs the buffer-truth so unsaved changes show in snippets.
    diagnostics::SourceFileCache::instance().publishFromContent(
        uri, doc->content);

    try {
        // Clear diagnostics but preserve AST and environment if parse fails
        doc->diagnostics.clear();
        doc->tokens.clear();

        // Save previous AST and environment in case parse fails
        std::vector<std::unique_ptr<ast::ASTNode>> previousAst;
        std::shared_ptr<environment::Environment> previousEnvironment;
        std::unordered_map<std::string, SymbolLocationInfo> previousSymbolLocations;

        if (!doc->ast.empty()) {
            // Keep a backup of the previous valid state
            // We can't move, so we'll just keep the old one if parsing fails
        }

        // Create an in-memory file reader for LSP
        auto fileReader = std::make_unique<MemoryFileReader>();
        fileReader->setContent(uri, doc->content);

        // Create lexer with our custom file reader
        doc->lexer = std::make_unique<Lexer>(uri, std::move(fileReader));

        // Note: Parser takes ownership of ImportManager, but we need to keep a reference
        // For LSP, we'll use a per-document ImportManager for now
        auto docImportManager = std::make_unique<ImportManager>();

        // Create parser with the lexer
        doc->parser = std::make_unique<Parser>(*doc->lexer, std::move(docImportManager));

        // Try to parse the document to get new AST
        std::vector<std::unique_ptr<ast::ASTNode>> newAst;
        bool parseSucceeded = false;

        try {
            auto program = doc->parser->parseProgram();
            if (program) {
                newAst.push_back(std::move(program));
                parseSucceeded = true;
            }
        } catch (const std::exception& e) {
            doc->diagnostics.push_back(diagnostics::fromException(e));
        }

        // Only update AST and environment if parsing succeeded
        if (parseSucceeded && !newAst.empty()) {
            doc->ast = std::move(newAst);

            // Create environment for semantic analysis
            doc->environment = EnvironmentBuilder::createDefault();
        } else if (!doc->environment) {
            // First parse and it failed - create empty environment
            doc->environment = EnvironmentBuilder::createDefault();
        }

        // Build symbol tables from AST using symbol registration (only if we have new AST)
        try {
            if (parseSucceeded && !doc->ast.empty()) {
                // Create symbol registration visitor
                auto visitor = std::make_unique<SymbolRegistrationVisitor>(doc->environment);

                // Traverse AST to register all symbols
                for (const auto& node : doc->ast) {
                    if (node) {
                        visitor->processProgram(node.get(), uri);
                    }
                }

                // Store symbol locations for go-to-definition
                doc->symbolLocations = visitor->getSymbolLocations();
            }
        } catch (const std::exception& e) {
            doc->diagnostics.push_back(diagnostics::fromException(e));
        }

        // MYT-50 LSP parity — run the missing-@Override checker against
        // every class registered in the document's environment. The check
        // is read-only and pure, identical to what ClassRegistrar runs on
        // the bytecode-compile path. Catches the case where a method
        // shadows a parent without `@Override` and surfaces an MT-W2002
        // warning in VS Code.
        if (doc->environment) {
            if (auto classRegistry = doc->environment->getClassRegistry()) {
                for (const auto& className : classRegistry->getAllItemNames()) {
                    auto cls = classRegistry->findClass(className);
                    if (!cls) continue;
                    auto warns = analysis::OverrideAnnotationChecker::check(*cls);
                    for (auto& w : warns) {
                        doc->diagnostics.push_back(std::move(w));
                    }
                }
            }
        }

        // MYT-49 — run the unused-variable analyzer. Conservative walker
        // (gives up on unfamiliar AST shapes so we never produce a false
        // positive) emits one MT-W2001 per declared-but-never-read local
        // with rename + remove fixes attached.
        if (parseSucceeded && !doc->ast.empty()) {
            auto unusedWarnings = analysis::UnusedVariableAnalyzer::analyze(
                doc->ast.front().get());
            for (auto& w : unusedWarnings) {
                doc->diagnostics.push_back(std::move(w));
            }
        }

        // Resolve and parse imported files to get their symbols
        if (doc->environment && importResolver_) {
            try {
                importResolver_->resolveImports(doc, this);
            } catch (const std::exception&) {
                // Silently ignore import resolution errors
            }
        }

        // Tokenize for token-based features (keep for completion)
        // Re-create lexer for tokenization
        auto tokenFileReader = std::make_unique<MemoryFileReader>();
        tokenFileReader->setContent(uri, doc->content);
        auto tokenLexer = std::make_unique<Lexer>(uri, std::move(tokenFileReader));

        Token token;
        do {
            token = tokenLexer->getNextToken();
            doc->tokens.push_back(token);
        } while (token.type != TokenType::END);

        doc->isParsed = true;

    } catch (const std::exception& e) {
        doc->diagnostics.push_back(diagnostics::fromException(e));
        doc->isParsed = false;
    }
}

std::string DocumentManager::getWordAtPosition(const std::string& uri, int line, int character) const {
    auto doc = getDocument(uri);
    if (!doc) return "";

    return extractWordAtPosition(doc->content, line, character);
}

std::string DocumentManager::extractWordAtPosition(const std::string& content, int line, int character) const {
    std::istringstream stream(content);
    std::string currentLine;
    int currentLineNum = 0;

    // Find the line
    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    if (currentLineNum != line || character >= static_cast<int>(currentLine.length())) {
        return "";
    }

    // Find word boundaries
    int start = character;
    int end = character;

    // Move start backward
    while (start > 0 && (std::isalnum(currentLine[start - 1]) || currentLine[start - 1] == '_')) {
        start--;
    }

    // Move end forward
    while (end < static_cast<int>(currentLine.length()) &&
           (std::isalnum(currentLine[end]) || currentLine[end] == '_')) {
        end++;
    }

    return currentLine.substr(start, end - start);
}

std::vector<std::string> DocumentManager::getIdentifiersInScope(const std::string& uri, int line) const {
    auto doc = getDocument(uri);
    if (!doc) {
        return {};
    }

    std::vector<std::string> identifiers;

    // Extract variable declarations using regex patterns
    // Pattern matches: TypeName variableName = ...
    std::regex varDeclPattern("([A-Z][a-zA-Z0-9_]*(?:<[^>]+>)?)\\s+([a-z][a-zA-Z0-9_]*)\\s*=");

    std::istringstream stream(doc->content);
    std::string currentLine;
    int currentLineNum = 0;

    while (std::getline(stream, currentLine) && currentLineNum <= line) {
        std::smatch match;
        std::string::const_iterator searchStart(currentLine.cbegin());

        while (std::regex_search(searchStart, currentLine.cend(), match, varDeclPattern)) {
            std::string varName = match[2].str();
            // Avoid duplicates
            if (std::find(identifiers.begin(), identifiers.end(), varName) == identifiers.end()) {
                identifiers.push_back(varName);
            }
            searchStart = match.suffix().first;
        }

        currentLineNum++;
    }

    // If we have an environment, also add variables from variable manager
    if (doc->isParsed && doc->environment && doc->environment->getVariableManager()) {
        auto vars = doc->environment->getVariableManager()->getAllVariableNames();
        for (const auto& var : vars) {
            if (std::find(identifiers.begin(), identifiers.end(), var) == identifiers.end()) {
                identifiers.push_back(var);
            }
        }
    }

    return identifiers;
}

std::optional<DocumentManager::SymbolLocation> DocumentManager::findDefinition(
    const std::string& uri, int line, int character) const {

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed || !doc->environment) {
        return std::nullopt;
    }

    // Get the current line to check for method calls
    std::istringstream stream(doc->content);
    std::string currentLine;
    int currentLineNum = 0;
    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    // Get the symbol name at the position
    std::string symbolName = extractWordAtPosition(doc->content, line, character);
    if (symbolName.empty()) {
        return std::nullopt;
    }

    // Check if this is a static member access (e.g., "ClassName::methodName(...)")
    // Look backwards from the character position to see if there's ::
    if (character > 1 && currentLineNum == line) {
        int colonPos = -1;

        // Look backwards through the current identifier and any whitespace to find ::
        for (int i = character - 1; i >= 1; i--) {
            if (currentLine[i] == ':' && currentLine[i - 1] == ':') {
                colonPos = i - 1; // Position of the first colon
                break;
            } else if (!std::isalnum(currentLine[i]) && currentLine[i] != '_' && !std::isspace(currentLine[i])) {
                // Found a character that's not part of an identifier, whitespace, or colon
                break;
            }
        }

        if (colonPos != -1) {
            // This is a static member access: extract the class name before ::
            int classStart = colonPos - 1;
            while (classStart >= 0 && (std::isalnum(currentLine[classStart]) || currentLine[classStart] == '_')) {
                classStart--;
            }
            classStart++;

            std::string className = currentLine.substr(classStart, colonPos - classStart);

            if (!className.empty()) {
                // Look for ClassName.memberName in symbol locations (static members use same format)
                std::string memberKey = className + "." + symbolName;

                auto it = doc->symbolLocations.find(memberKey);
                if (it != doc->symbolLocations.end()) {
                    const auto& symbolLoc = it->second;
                    SymbolLocation result;
                    result.uri = symbolLoc.uri;
                    result.line = symbolLoc.line;
                    result.column = symbolLoc.column;
                    return result;
                }
            }
        }
    }

    // Check if this is a method call (e.g., "variable.methodName(...)")
    // Look backwards from the character position to see if there's a dot
    if (character > 0 && currentLineNum == line) {
        int dotPos = -1;

        // First, look backwards through the current identifier and any whitespace to find a dot
        for (int i = character - 1; i >= 0; i--) {
            if (currentLine[i] == '.') {
                dotPos = i;
                break;
            } else if (!std::isalnum(currentLine[i]) && currentLine[i] != '_' && !std::isspace(currentLine[i])) {
                // Found a character that's not part of an identifier, whitespace, or a dot
                // This means we're not in a method call
                break;
            }
        }

        if (dotPos != -1) {
            // This is a method call: extract the variable name before the dot
            int varStart = dotPos - 1;
            while (varStart >= 0 && (std::isalnum(currentLine[varStart]) || currentLine[varStart] == '_')) {
                varStart--;
            }
            varStart++;

            std::string varName = currentLine.substr(varStart, dotPos - varStart);

            // Infer the type of this variable by scanning the document
            std::string varType = inferVariableType(doc->content, varName);

            if (!varType.empty()) {
                // Look for ClassName.methodName in symbol locations
                std::string methodKey = varType + "." + symbolName;

                auto it = doc->symbolLocations.find(methodKey);
                if (it != doc->symbolLocations.end()) {
                    const auto& symbolLoc = it->second;
                    SymbolLocation result;
                    result.uri = symbolLoc.uri;
                    result.line = symbolLoc.line;
                    result.column = symbolLoc.column;
                    return result;
                }
            }
        }
    }

    // Fall back to looking up the symbol directly (for classes, interfaces, functions)
    auto it = doc->symbolLocations.find(symbolName);
    if (it != doc->symbolLocations.end()) {
        const auto& symbolLoc = it->second;
        SymbolLocation result;
        result.uri = symbolLoc.uri;
        result.line = symbolLoc.line;
        result.column = symbolLoc.column;
        return result;
    }

    return std::nullopt;
}

std::string DocumentManager::renderValueTypeName(
    value::ValueType vt, const std::string& className) const {
    switch (vt) {
        case value::ValueType::INT: return "int";
        case value::ValueType::FLOAT: return "float";
        case value::ValueType::BOOL: return "bool";
        case value::ValueType::STRING: return "string";
        case value::ValueType::VOID: return "void";
        case value::ValueType::OBJECT:
            return className.empty() ? "object" : className;
        case value::ValueType::LAMBDA: return "lambda";
        case value::ValueType::ARRAY:
            return className.empty() ? "array" : className;
        case value::ValueType::NULL_TYPE: return "null";
        default: return "unknown";
    }
}

DocumentManager::CallContext DocumentManager::classifyCallContext(
    const std::string& content, int line, int character) const {
    CallContext ctx;

    std::istringstream stream(content);
    std::string currentLine;
    int currentLineNum = 0;
    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }
    if (currentLineNum != line) return ctx;

    int lineLen = static_cast<int>(currentLine.length());
    if (character < 0 || character > lineLen) return ctx;

    // Walk left to the start of the hovered word.
    int wordStart = character;
    while (wordStart > 0
           && (std::isalnum(static_cast<unsigned char>(currentLine[wordStart - 1]))
               || currentLine[wordStart - 1] == '_')) {
        wordStart--;
    }

    // Skip whitespace before the word.
    int i = wordStart - 1;
    while (i >= 0 && std::isspace(static_cast<unsigned char>(currentLine[i]))) i--;
    if (i < 0) return ctx;

    // `::` lookbehind -> StaticMethod
    if (i >= 1 && currentLine[i] == ':' && currentLine[i - 1] == ':') {
        int idEnd = i - 2;
        while (idEnd >= 0 && std::isspace(static_cast<unsigned char>(currentLine[idEnd]))) idEnd--;
        int idStart = idEnd;
        while (idStart >= 0
               && (std::isalnum(static_cast<unsigned char>(currentLine[idStart]))
                   || currentLine[idStart] == '_')) {
            idStart--;
        }
        if (idEnd > idStart) {
            ctx.kind = CallContext::Kind::StaticMethod;
            ctx.receiver = currentLine.substr(idStart + 1, idEnd - idStart);
        }
        return ctx;
    }

    // `.` lookbehind -> Method
    if (currentLine[i] == '.') {
        int idEnd = i - 1;
        while (idEnd >= 0 && std::isspace(static_cast<unsigned char>(currentLine[idEnd]))) idEnd--;
        int idStart = idEnd;
        while (idStart >= 0
               && (std::isalnum(static_cast<unsigned char>(currentLine[idStart]))
                   || currentLine[idStart] == '_')) {
            idStart--;
        }
        if (idEnd > idStart) {
            ctx.kind = CallContext::Kind::Method;
            ctx.receiver = currentLine.substr(idStart + 1, idEnd - idStart);
        }
        return ctx;
    }

    // Preceding identifier -> check for `new` keyword
    int idEnd = i;
    int idStart = idEnd;
    while (idStart >= 0
           && (std::isalnum(static_cast<unsigned char>(currentLine[idStart]))
               || currentLine[idStart] == '_')) {
        idStart--;
    }
    if (idEnd > idStart) {
        std::string prev = currentLine.substr(idStart + 1, idEnd - idStart);
        if (prev == "new") {
            ctx.kind = CallContext::Kind::Constructor;
        }
    }
    return ctx;
}

std::string DocumentManager::formatFunctionSignature(
    const runtimeTypes::global::FunctionDefinition& fn) const {
    std::string out;
    if (fn.getIsAsync()) out += "async ";
    out += "function " + fn.getName() + "(";
    const auto& params = fn.getParameters();
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) out += ", ";
        out += params[i].second.toString() + " " + params[i].first;
    }
    out += ") : " + renderValueTypeName(fn.getReturnType(), fn.getReturnClassName());
    return out;
}

std::string DocumentManager::formatMethodSignature(
    const std::string& ownerClass,
    const runtimeTypes::klass::MethodDefinition& m) const {
    std::string out;
    out += std::string(ast::accessModifierToString(m.getAccessModifier())) + " ";
    if (m.isStatic()) out += "static ";
    if (m.getIsAsync()) out += "async ";
    if (m.isAbstract()) out += "abstract ";
    if (m.isFinal()) out += "final ";
    out += ownerClass + "::" + m.getName() + "(";

    const auto& params = m.getParameters();
    size_t startIdx = 0;
    // Skip implicit leading `this` parameter on instance methods (matches
    // InlayHintHandler's lookup convention).
    if (!m.isStatic() && !params.empty() && params.front().first == "this") {
        startIdx = 1;
    }
    bool first = true;
    for (size_t i = startIdx; i < params.size(); ++i) {
        if (!first) out += ", ";
        first = false;
        out += params[i].second.toString() + " " + params[i].first;
    }
    out += ") : ";

    // Prefer the unified return type when present (carries class names);
    // fall back to the basic ValueType switch when only the legacy field is set.
    if (auto unifiedRet = m.getUnifiedReturnType()) {
        out += unifiedRet->toString();
    } else {
        out += renderValueTypeName(m.getReturnType(), "");
    }
    return out;
}

std::string DocumentManager::formatConstructorSignature(
    const std::string& className,
    const runtimeTypes::klass::ConstructorDefinition& ctor) const {
    std::string out;
    out += std::string(ast::accessModifierToString(ctor.getAccessModifier())) + " ";
    out += className + "(";
    const auto& params = ctor.getParametersWithTypes();
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) out += ", ";
        out += params[i].second.toString() + " " + params[i].first;
    }
    out += ")";
    return out;
}

std::string DocumentManager::formatClassHover(
    const runtimeTypes::klass::ClassDefinition& cls) const {
    std::string out = "class " + cls.getName();

    const auto& generics = cls.getGenericParameters();
    if (!generics.empty()) {
        out += "<";
        for (size_t i = 0; i < generics.size(); ++i) {
            if (i > 0) out += ", ";
            out += generics[i].name;
        }
        out += ">";
    }

    if (cls.hasParentClass()) {
        out += " extends " + cls.getParentClassName();
    }

    const auto& interfaces = cls.getImplementedInterfaces();
    if (!interfaces.empty()) {
        out += " implements ";
        for (size_t i = 0; i < interfaces.size(); ++i) {
            if (i > 0) out += ", ";
            out += interfaces[i];
        }
    }

    // Fields in declaration order.
    const auto& fieldOrder = cls.getInstanceFieldOrder();
    const auto& instanceFields = cls.getInstanceFields();
    for (const auto& fieldName : fieldOrder) {
        auto it = instanceFields.find(fieldName);
        if (it == instanceFields.end() || !it->second) continue;
        const auto& f = *it->second;
        out += "\n    " + f.getName() + ": " + renderValueTypeName(f.getType(), "");
    }

    // Constructors.
    for (const auto& ctor : cls.getConstructors()) {
        if (!ctor) continue;
        out += "\n    " + formatConstructorSignature(cls.getName(), *ctor);
    }

    // Method signatures in declaration order; hide compiler-synthesized ones
    // (auto hashCode / equals etc., per ClassDefinition.hpp:40-48). Hard-cap
    // at 10 lines and append `... (N more)` when truncated.
    constexpr size_t METHOD_CAP = 10;
    const auto& methodOrder = cls.getInstanceMethodOrder();
    const auto& instanceMethods = cls.getInstanceMethods();
    std::vector<std::string> methodLines;
    for (const auto& mname : methodOrder) {
        auto it = instanceMethods.find(mname);
        if (it == instanceMethods.end()) continue;
        for (const auto& m : it->second) {
            if (!m || m->isSynthetic()) continue;
            methodLines.push_back(formatMethodSignature(cls.getName(), *m));
        }
    }
    for (size_t i = 0; i < methodLines.size(); ++i) {
        if (i >= METHOD_CAP) {
            out += "\n    ... (" + std::to_string(methodLines.size() - METHOD_CAP) + " more)";
            break;
        }
        out += "\n    " + methodLines[i];
    }

    return out;
}

std::string DocumentManager::formatInterfaceHover(
    const runtimeTypes::klass::InterfaceDefinition& iface) const {
    std::string out = "interface " + iface.getName();

    const auto& generics = iface.getGenericParameters();
    if (!generics.empty()) {
        out += "<";
        for (size_t i = 0; i < generics.size(); ++i) {
            if (i > 0) out += ", ";
            out += generics[i].name;
        }
        out += ">";
    }

    const auto& exts = iface.getExtendedInterfaces();
    if (!exts.empty()) {
        out += " extends ";
        for (size_t i = 0; i < exts.size(); ++i) {
            if (i > 0) out += ", ";
            out += exts[i];
        }
    }

    for (const auto& sig : iface.getMethodSignatures()) {
        out += "\n    " + sig.name + "(";
        for (size_t i = 0; i < sig.parameters.size(); ++i) {
            if (i > 0) out += ", ";
            const auto& [pname, ptype] = sig.parameters[i];
            std::string typeStr = ptype ? ptype->toString() : std::string("?");
            out += typeStr + " " + pname;
        }
        out += ")";
        if (sig.returnType) {
            out += " : " + sig.returnType->toString();
        }
    }
    return out;
}

std::optional<std::string> DocumentManager::getTypeInfo(
    const std::string& uri, int line, int character) const {

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed || !doc->environment) {
        return std::nullopt;
    }

    // Get the symbol name at the position
    std::string symbolName = extractWordAtPosition(doc->content, line, character);
    if (symbolName.empty()) {
        return std::nullopt;
    }

    // Try to get type information from environment
    try {
        // Check for variables (unchanged from pre-MYT-357 behavior)
        auto varDef = doc->environment->findVariable(symbolName);
        if (varDef) {
            return symbolName + ": "
                + renderValueTypeName(varDef->getType(), varDef->getClassName());
        }

        // Classify call site once so each branch can route precisely.
        CallContext ctx = classifyCallContext(doc->content, line, character);
        auto classReg = doc->environment->getClassRegistry();
        auto funcReg = doc->environment->getFunctionRegistry();

        // `new Foo(...)` — list every constructor of Foo. Fall through to the
        // bare class lookup below when Foo has no explicit constructors.
        if (ctx.kind == CallContext::Kind::Constructor && classReg) {
            auto cls = classReg->findClass(symbolName);
            if (cls && !cls->getConstructors().empty()) {
                std::string out;
                for (const auto& ctor : cls->getConstructors()) {
                    if (!ctor) continue;
                    if (!out.empty()) out += "\n";
                    out += formatConstructorSignature(symbolName, *ctor);
                }
                if (!out.empty()) return out;
            }
        }

        // `obj.method(...)` — resolve receiver's class via inferVariableType.
        // If the receiver looks like a class (starts upper-case) we treat it
        // as a class name directly, which covers `Foo.staticField` style.
        if (ctx.kind == CallContext::Kind::Method && classReg) {
            std::string receiverType = inferVariableType(doc->content, ctx.receiver);
            if (receiverType.empty() && !ctx.receiver.empty()
                && std::isupper(static_cast<unsigned char>(ctx.receiver.front()))) {
                receiverType = ctx.receiver;
            }
            if (!receiverType.empty()) {
                auto lt = receiverType.find('<');
                if (lt != std::string::npos) receiverType = receiverType.substr(0, lt);
                auto cls = classReg->findClass(receiverType);
                if (cls) {
                    auto overloads = cls->getAllInstanceMethodOverloads(symbolName);
                    if (overloads.empty()) {
                        overloads = cls->getAllStaticMethodOverloads(symbolName);
                    }
                    std::string out;
                    for (const auto& m : overloads) {
                        if (!m) continue;
                        if (!out.empty()) out += "\n";
                        out += formatMethodSignature(cls->getName(), *m);
                    }
                    if (!out.empty()) return out;
                }

                // Receiver typed as an interface (e.g. `Function mult = ...; mult.apply(x);`).
                // ClassRegistry misses these — fall back to InterfaceRegistry and
                // render any matching abstract method signature.
                if (auto iface = doc->environment->findInterface(receiverType)) {
                    std::string out;
                    for (const auto& sig : iface->getMethodSignatures()) {
                        if (sig.name != symbolName) continue;
                        std::string line = iface->getName() + "::" + sig.name + "(";
                        for (size_t i = 0; i < sig.parameters.size(); ++i) {
                            if (i > 0) line += ", ";
                            const auto& [pname, ptype] = sig.parameters[i];
                            std::string typeStr = ptype ? ptype->toString() : std::string("?");
                            line += typeStr + " " + pname;
                        }
                        line += ")";
                        if (sig.returnType) line += " : " + sig.returnType->toString();
                        if (!out.empty()) out += "\n";
                        out += line;
                    }
                    if (!out.empty()) return out;
                }
            }
        }

        // `Class::method(...)` — static-method lookup; fall back to instance
        // lookup in case the receiver was upper-case but the method is
        // actually instance-side.
        if (ctx.kind == CallContext::Kind::StaticMethod && classReg && !ctx.receiver.empty()) {
            auto cls = classReg->findClass(ctx.receiver);
            if (cls) {
                auto overloads = cls->getAllStaticMethodOverloads(symbolName);
                if (overloads.empty()) {
                    overloads = cls->getAllInstanceMethodOverloads(symbolName);
                }
                std::string out;
                for (const auto& m : overloads) {
                    if (!m) continue;
                    if (!out.empty()) out += "\n";
                    out += formatMethodSignature(cls->getName(), *m);
                }
                if (!out.empty()) return out;
            }
        }

        // Bare function — list every overload registered under this name.
        if (funcReg) {
            auto overloads = funcReg->getAllFunctionOverloads(symbolName);
            std::string out;
            for (const auto& fn : overloads) {
                if (!fn) continue;
                if (!out.empty()) out += "\n";
                out += formatFunctionSignature(*fn);
            }
            if (!out.empty()) return out;
        }

        // Bare class -> full class summary.
        if (classReg) {
            auto cls = classReg->findClass(symbolName);
            if (cls) return formatClassHover(*cls);
        }

        // Bare interface -> interface summary.
        auto iface = doc->environment->findInterface(symbolName);
        if (iface) return formatInterfaceHover(*iface);

    } catch (...) {
        // Ignore errors
    }

    return std::nullopt;
}

std::vector<DocumentManager::SymbolInfo> DocumentManager::getDocumentSymbols(const std::string& uri) const {
    std::vector<SymbolInfo> symbols;

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed) {
        return symbols;
    }

    // Extract symbols from the symbol locations map populated during parsing
    for (const auto& [symbolName, location] : doc->symbolLocations) {
        SymbolInfo info;
        info.name = symbolName;
        info.line = location.line;
        info.column = location.column;

        // Determine symbol kind by checking the environment
        try {
            // Check if it's a class
            if (doc->environment && doc->environment->getClassRegistry()) {
                if (doc->environment->getClassRegistry()->hasClass(symbolName)) {
                    info.kind = "class";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Check if it's a function
            if (doc->environment && doc->environment->getFunctionRegistry()) {
                auto funcDef = doc->environment->getFunctionRegistry()->findFunction(symbolName);
                if (funcDef) {
                    info.kind = "function";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Check if it's a variable
            if (doc->environment) {
                auto varDef = doc->environment->findVariable(symbolName);
                if (varDef) {
                    info.kind = "variable";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Default to "symbol" if we can't determine the kind
            info.kind = "symbol";
            symbols.push_back(info);

        } catch (...) {
            // Ignore errors, skip this symbol
        }
    }

    return symbols;
}

std::string DocumentManager::inferVariableType(const std::string& content, const std::string& varName) const {
    // Pattern 1: ClassName<GenericType> varName = new ClassName<GenericType>(...)
    // Pattern 2: ClassName<GenericType> varName = ...
    // Pattern 3: for (ClassName varName : ...) - for-each loop variable
    // Pattern 4: for (ClassName<GenericType> varName : ...) - for-each loop with generic type
    // Note: <GenericType> is optional, handles both generic and non-generic types
    // Captures only the base class name without generic parameters
    std::regex declPattern1("([A-Z][a-zA-Z0-9_]*)(?:<[^>]+>)?\\s+" + varName + "\\s*=\\s*new\\s+([A-Z][a-zA-Z0-9_]*)");
    std::regex declPattern2("([A-Z][a-zA-Z0-9_]*)(?:<[^>]+>)?\\s+" + varName + "\\s*=");
    std::regex forEachPattern("for\\s*\\(\\s*([A-Z][a-zA-Z0-9_]*)(?:<[^>]+>)?\\s+" + varName + "\\s*:");

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;

        // Try pattern 1: with "new" keyword
        if (std::regex_search(line, match, declPattern1)) {
            // Prefer the type from "new" if it matches the declaration type
            std::string newType = match[2].str();
            return newType; // Return the type from "new" as it's the actual instantiated type
        }

        // Try pattern 2: simple declaration
        if (std::regex_search(line, match, declPattern2)) {
            std::string declType = match[1].str();
            return declType;
        }

        // Try pattern 3: for-each loop variable
        if (std::regex_search(line, match, forEachPattern)) {
            std::string loopVarType = match[1].str();
            return loopVarType;
        }
    }

    return ""; // Type not found
}

std::string DocumentManager::getVariableType(const std::string& uri, const std::string& varName, int line) const {
    auto doc = getDocument(uri);
    if (!doc) {
        return "";
    }

    // Use the existing inferVariableType method
    return inferVariableType(doc->content, varName);
}

} // namespace mtype::lsp
