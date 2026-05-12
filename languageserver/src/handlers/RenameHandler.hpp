#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

// MYT-294 — Safe symbol rename.
//
// Two design rules separate this from ReferencesHandler:
//   1. Occurrences come from the lexer's already-produced token stream
//      (Document::tokens) and only IDENTIFIER tokens are considered.
//      This excludes comments (the lexer drops them), string/number
//      literals, keywords, and operators by construction — no regex
//      over raw source text.
//   2. prepareRename rejects keywords, builtins (anything defined under
//      `/lib/`), tokens whose source span is a non-IDENTIFIER type, and
//      symbols whose identity cannot be made unambiguous in v1
//      (e.g., a member name shared by two unrelated classes).
class RenameHandler {
public:
    explicit RenameHandler(DocumentManager* docMgr);

    struct PrepareResult {
        bool ok = false;
        Range range{};
        std::string error;
    };

    struct RenameResult {
        bool ok = false;
        WorkspaceEdit edit{};
        std::string error;
    };

    PrepareResult prepareRename(const std::string& uri, const Position& position);

    RenameResult rename(const std::string& uri,
                        const Position& position,
                        const std::string& newName);

private:
    enum class SymbolKind { TopLevel, Member, Local };

    struct TargetInfo {
        std::string name;
        SymbolKind kind = SymbolKind::Local;
        std::string declaringClass;   // for Member
        std::string defUri;            // file URI where the symbol is declared
        bool isBuiltin = false;
        Range cursorRange{};           // identifier span at the cursor
    };

    // Identifier rules — ASCII-only per MYT-294 v1 scope.
    static bool isValidIdentifier(const std::string& s);
    static bool isReservedKeyword(const std::string& s);
    static bool isAsciiLine(const std::string& line);

    // Built-in heuristic: any URI whose path contains a "/lib/" segment
    // (after backslash normalisation) refers to a stdlib file and is
    // therefore not renameable.
    static bool isUriUnderLib(const std::string& uri);

    // Locate the token whose [start..end) source range covers the cursor.
    // Both arguments are LSP-style 0-based positions. Returns nullptr if
    // the cursor is on whitespace, between tokens, or out of range.
    const token::Token* findIdentifierTokenAt(
        const std::vector<token::Token>& tokens,
        int line,
        int character) const;

    // Classify the symbol the cursor points at. Fills `out` and returns
    // true on success; on failure returns false and writes a
    // user-facing message into `error`.
    bool classifyTarget(const std::string& uri,
                        const Position& position,
                        TargetInfo& out,
                        std::string& error);

    // For a Local target, ensure the name is declared exactly once in
    // the same file. Heuristic: count IDENTIFIER tokens whose previous
    // non-trivia token is a type token (int/float/bool/string/object
    // keyword or another IDENTIFIER). If more than one declaration is
    // found, rename is ambiguous and we reject.
    bool localIsUnambiguous(const Document* doc,
                            const std::string& name,
                            std::string& error) const;

    // For a Member target, ensure the symbol name is not also declared
    // as a member in another class outside the override set
    // (declaring class + its ancestors + its descendants).
    bool memberIsUnambiguous(const std::string& memberName,
                             const std::string& declaringClass,
                             std::string& error) const;

    // For a TopLevel target, ensure no other workspace top-level
    // declaration shares this name.
    bool topLevelIsUnambiguous(const std::string& name,
                                const std::string& defUri,
                                std::string& error) const;

    // The set of classes that share a method/field name with the
    // declaring class through inheritance (declaring class + ancestors
    // + descendants, transitively).
    std::vector<std::string> collectOverrideSet(const std::string& declaringClass) const;

    // Collect IDENTIFIER-token ranges in `doc` whose lexeme equals
    // `name`. For Local targets the result is unfiltered (whole-file);
    // for TopLevel/Member targets the caller is responsible for any
    // member-receiver filtering above (v1 relies on the ambiguity
    // gate, see memberIsUnambiguous).
    std::vector<Range> collectOccurrencesInDoc(const Document* doc,
                                                const std::string& name) const;

    // Convert a token's 1-based source location to an LSP-style 0-based
    // Range. Token lexeme length is taken from stringValue.
    static Range tokenRange(const token::Token& tok);

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
