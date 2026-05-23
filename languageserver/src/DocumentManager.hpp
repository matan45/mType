#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Include necessary headers
#include "../../mType/token/Token.hpp"
#include "../../mType/lexer/Lexer.hpp"
#include "../../mType/parser/Parser.hpp"
#include "../../mType/environment/Environment.hpp"
#include "../../mType/services/ImportManager.hpp"
#include "../../mType/ast/ASTNode.hpp"
#include "../../mType/diagnostics/Diagnostic.hpp"

namespace mtype::lsp {

// Forward declarations
class SymbolRegistrationVisitor;
class ImportResolver;
class ProjectConfigProvider;

// Symbol location tracking for go-to-definition
struct SymbolLocationInfo {
    std::string uri;
    int line;
    int column;
    std::string className; // For methods: which class they belong to (empty for top-level symbols)
};

struct Document {
    std::string uri;
    std::string content;
    int version;

    // Parsed representation
    std::unique_ptr<lexer::Lexer> lexer;
    std::unique_ptr<parser::Parser> parser;
    std::shared_ptr<environment::Environment> environment;
    std::vector<std::unique_ptr<ast::ASTNode>> ast;

    // State
    bool isParsed = false;
    // MYT-35 — diagnostics are stored as the shared core model. Both
    // parser and semantic-analysis exceptions get converted at the
    // catch site (DocumentManager::parseDocument) via
    // diagnostics::fromException, so the LSP DiagnosticsHandler can
    // emit precise locations and codes without ever string-parsing
    // an exception's `what()`.
    std::vector<diagnostics::Diagnostic> diagnostics;
    std::vector<token::Token> tokens;

    // Symbol location tracking for go-to-definition
    std::unordered_map<std::string, SymbolLocationInfo> symbolLocations;
};

class DocumentManager {
public:
    DocumentManager();
    ~DocumentManager(); // Explicitly declared to handle unique_ptr with forward-declared type

    void openDocument(const std::string& uri, const std::string& content, int version);
    void updateDocument(const std::string& uri, const std::string& content, int version);
    void closeDocument(const std::string& uri);

    void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config);

    Document* getDocument(const std::string& uri);
    const Document* getDocument(const std::string& uri) const;

    // MYT-294 — enumerate every open document URI. Rename uses this to
    // walk all live editor buffers when computing cross-file edits;
    // the workspace symbol index alone is not enough because it only
    // tracks files that *declare* top-level symbols, missing files
    // that only *use* the symbol being renamed.
    std::vector<std::string> getAllOpenUris() const;

    // Parse document with full semantic analysis
    void parseDocument(const std::string& uri);

    // Query methods
    std::string getWordAtPosition(const std::string& uri, int line, int character) const;
    std::vector<std::string> getIdentifiersInScope(const std::string& uri, int line) const;
    std::string getVariableType(const std::string& uri, const std::string& varName, int line) const;

    // Semantic query methods
    struct SymbolLocation {
        std::string uri;
        int line;
        int column;
    };

    // Find the definition of a symbol at the given position
    std::optional<SymbolLocation> findDefinition(const std::string& uri, int line, int character) const;

    // Get type information for a symbol at the given position
    std::optional<std::string> getTypeInfo(const std::string& uri, int line, int character) const;

    // Get all symbols in a document
    struct SymbolInfo {
        std::string name;
        std::string kind; // "class", "function", "variable", etc.
        int line;
        int column;
    };
    std::vector<SymbolInfo> getDocumentSymbols(const std::string& uri) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Document>> documents_;
    std::unique_ptr<ImportResolver> importResolver_;

    std::string extractWordAtPosition(const std::string& content, int line, int character) const;
    std::string inferVariableType(const std::string& content, const std::string& varName) const;

    // MYT-357 — rich hover formatting helpers. classifyCallContext probes the
    // text immediately preceding the hovered word so getTypeInfo can route to
    // the right registry: `new X(...)`, `obj.x`, and `Class::x` each map to a
    // different lookup. Formatters live next to getTypeInfo because the hover
    // path is the only consumer; SignatureHelp keeps its own renderer.
    struct CallContext {
        enum class Kind { Bare, Constructor, Method, StaticMethod };
        Kind kind = Kind::Bare;
        std::string receiver;
    };

    CallContext classifyCallContext(const std::string& content, int line, int character) const;
    std::string renderValueTypeName(value::ValueType vt, const std::string& className) const;
    std::string formatFunctionSignature(const runtimeTypes::global::FunctionDefinition& fn) const;
    std::string formatMethodSignature(const std::string& ownerClass,
                                      const runtimeTypes::klass::MethodDefinition& m) const;
    std::string formatConstructorSignature(const std::string& className,
                                           const runtimeTypes::klass::ConstructorDefinition& ctor) const;
    std::string formatClassHover(const runtimeTypes::klass::ClassDefinition& cls) const;
    std::string formatInterfaceHover(const runtimeTypes::klass::InterfaceDefinition& iface) const;
};

} // namespace mtype::lsp
