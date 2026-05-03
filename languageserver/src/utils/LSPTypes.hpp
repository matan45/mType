#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mtype::lsp {

struct Position {
    int line;
    int character;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, line, character)
};

struct Range {
    Position start;
    Position end;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Range, start, end)
};

struct Location {
    std::string uri;
    Range range;

    json toJson() const {
        json j = {
            {"uri", uri},
            {"range", range}
        };
        return j;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Location, uri, range)
};

struct TextDocumentIdentifier {
    std::string uri;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentIdentifier, uri)
};

struct TextDocumentItem {
    std::string uri;
    std::string languageId;
    int version;
    std::string text;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentItem, uri, languageId, version, text)
};

struct VersionedTextDocumentIdentifier {
    std::string uri;
    int version;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VersionedTextDocumentIdentifier, uri, version)
};

struct TextDocumentContentChangeEvent {
    std::optional<Range> range;
    std::string text;
};

inline void from_json(const json& j, TextDocumentContentChangeEvent& e) {
    if (j.contains("range")) {
        e.range = j.at("range").get<Range>();
    }
    j.at("text").get_to(e.text);
}

struct TextEdit {
    Range range;
    std::string newText;

    json toJson() const {
        return json{
            {"range", range},
            {"newText", newText}
        };
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextEdit, range, newText)
};

struct CompletionItem {
    std::string label;
    int kind; // CompletionItemKind
    std::optional<std::string> detail;
    std::optional<std::string> documentation;
    std::optional<std::string> insertText;
    std::optional<int> insertTextFormat; // 1 = plain text, 2 = snippet
    std::optional<std::string> sortText;
    std::optional<std::string> filterText;
    std::optional<TextEdit> textEdit;
    // MYT-51 — auto-import completion items attach a TextEdit that
    // inserts the missing `import ... from "..."` line when the user
    // accepts the completion. LSP spec models this as an array; VS
    // Code applies all entries silently alongside the main insert.
    std::vector<TextEdit> additionalTextEdits;
    // Per-item characters that, when typed by the user, commit this
    // completion before the trailing character is inserted. Methods
    // and functions set this to ["("] so typing the open paren
    // accepts the suggestion in one motion.
    std::vector<std::string> commitCharacters;
    // Opaque blob round-tripped through completionItem/resolve so the
    // server can recover the original symbol identity (kind + fqName)
    // when populating documentation lazily.
    std::optional<json> data;

    json toJson() const;
};

enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19
};

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct CodeDescription {
    std::string href;

    json toJson() const {
        return json{ {"href", href} };
    }
};

inline void from_json(const json& j, CodeDescription& cd) {
    j.at("href").get_to(cd.href);
}

struct DiagnosticRelatedInformation {
    Location location;
    std::string message;

    json toJson() const {
        return json{
            {"location", location.toJson()},
            {"message", message}
        };
    }
};

inline void from_json(const json& j, DiagnosticRelatedInformation& dri) {
    j.at("location").get_to(dri.location);
    j.at("message").get_to(dri.message);
}

struct Diagnostic {
    Range range;
    int severity; // DiagnosticSeverity
    std::string message;
    std::optional<std::string> source;

    // Extended LSP fields used by the unified diagnostic system (MYT-35).
    // `code` is a string (we publish "MT-E1001"-style ids) but the spec
    // also allows int — we store as json so round-tripping works either way.
    std::optional<json> code;
    std::optional<CodeDescription> codeDescription;
    std::vector<DiagnosticRelatedInformation> relatedInformation;
    std::vector<int> tags;     // DiagnosticTag: Unnecessary=1, Deprecated=2
    // Opaque blob round-tripped through textDocument/codeAction so the
    // server can recover the original diagnostic context when generating
    // quick fixes.
    std::optional<json> data;

    json toJson() const {
        json j = {
            {"range", range},
            {"severity", severity},
            {"message", message}
        };
        if (source) j["source"] = *source;
        if (code) j["code"] = *code;
        if (codeDescription) j["codeDescription"] = codeDescription->toJson();
        if (!relatedInformation.empty()) {
            json arr = json::array();
            for (const auto& ri : relatedInformation) arr.push_back(ri.toJson());
            j["relatedInformation"] = arr;
        }
        if (!tags.empty()) j["tags"] = tags;
        if (data) j["data"] = *data;
        return j;
    }
};

inline void from_json(const json& j, Diagnostic& d) {
    j.at("range").get_to(d.range);
    if (j.contains("severity")) {
        d.severity = j.at("severity").get<int>();
    } else {
        d.severity = 1; // Error
    }
    if (j.contains("message")) {
        d.message = j.at("message").get<std::string>();
    }
    if (j.contains("source")) {
        d.source = j.at("source").get<std::string>();
    }
    if (j.contains("code")) {
        d.code = j.at("code");
    }
    if (j.contains("codeDescription")) {
        d.codeDescription = j.at("codeDescription").get<CodeDescription>();
    }
    if (j.contains("relatedInformation")) {
        for (const auto& item : j.at("relatedInformation")) {
            d.relatedInformation.push_back(item.get<DiagnosticRelatedInformation>());
        }
    }
    if (j.contains("tags")) {
        for (const auto& t : j.at("tags")) {
            d.tags.push_back(t.get<int>());
        }
    }
    if (j.contains("data")) {
        d.data = j.at("data");
    }
}

struct Hover {
    std::string contents;
    std::optional<Range> range;

    json toJson() const {
        json j = {
            {"contents", contents}
        };
        if (range) j["range"] = *range;
        return j;
    }
};

inline json CompletionItem::toJson() const {
    json j = {
        {"label", label},
        {"kind", kind}
    };
    if (detail) j["detail"] = *detail;
    if (documentation) j["documentation"] = *documentation;
    if (insertText) j["insertText"] = *insertText;
    if (insertTextFormat) j["insertTextFormat"] = *insertTextFormat;
    if (sortText) j["sortText"] = *sortText;
    if (filterText) j["filterText"] = *filterText;
    if (textEdit) j["textEdit"] = textEdit->toJson();
    if (!additionalTextEdits.empty()) {
        json arr = json::array();
        for (const auto& te : additionalTextEdits) arr.push_back(te.toJson());
        j["additionalTextEdits"] = arr;
    }
    if (!commitCharacters.empty()) j["commitCharacters"] = commitCharacters;
    if (data) j["data"] = *data;
    return j;
}

// Round-trip CompletionItem through completionItem/resolve. VS Code echoes
// the full object (including the opaque `data` blob and any fields it
// preserved) so the server can rehydrate the original symbol identity
// without recomputing the whole completion list.
inline void from_json(const json& j, CompletionItem& item) {
    j.at("label").get_to(item.label);
    j.at("kind").get_to(item.kind);
    if (j.contains("detail") && j.at("detail").is_string()) {
        item.detail = j.at("detail").get<std::string>();
    }
    if (j.contains("documentation") && j.at("documentation").is_string()) {
        item.documentation = j.at("documentation").get<std::string>();
    }
    if (j.contains("insertText") && j.at("insertText").is_string()) {
        item.insertText = j.at("insertText").get<std::string>();
    }
    if (j.contains("insertTextFormat") && j.at("insertTextFormat").is_number_integer()) {
        item.insertTextFormat = j.at("insertTextFormat").get<int>();
    }
    if (j.contains("sortText") && j.at("sortText").is_string()) {
        item.sortText = j.at("sortText").get<std::string>();
    }
    if (j.contains("filterText") && j.at("filterText").is_string()) {
        item.filterText = j.at("filterText").get<std::string>();
    }
    if (j.contains("data")) {
        item.data = j.at("data");
    }
}

struct WorkspaceEdit {
    std::map<std::string, std::vector<TextEdit>> changes; // uri -> edits

    json toJson() const {
        json changesJson = json::object();
        for (const auto& [uri, edits] : changes) {
            json editsArray = json::array();
            for (const auto& edit : edits) {
                editsArray.push_back(edit.toJson());
            }
            changesJson[uri] = editsArray;
        }
        return json{
            {"changes", changesJson}
        };
    }
};

struct CodeAction {
    std::string title;
    std::optional<std::string> kind; // "quickfix", "refactor", etc.
    std::optional<WorkspaceEdit> edit;
    // The diagnostics that this action addresses (LSP spec field).
    // VS Code uses this to associate the action with the squiggle that
    // produced it, so the bulb shows up next to the right line.
    std::vector<Diagnostic> diagnostics;

    json toJson() const {
        json j = {
            {"title", title}
        };
        if (kind) j["kind"] = *kind;
        if (edit) j["edit"] = edit->toJson();
        if (!diagnostics.empty()) {
            json arr = json::array();
            for (const auto& d : diagnostics) arr.push_back(d.toJson());
            j["diagnostics"] = arr;
        }
        return j;
    }
};

struct Command {
    std::string title;
    std::string command;
    std::optional<json> arguments;

    json toJson() const {
        json j = {
            {"title", title},
            {"command", command}
        };
        if (arguments) j["arguments"] = *arguments;
        return j;
    }
};

struct CodeLens {
    Range range;
    std::optional<Command> command;
    std::optional<json> data;

    json toJson() const {
        json j = {
            {"range", range}
        };
        if (command) j["command"] = command->toJson();
        if (data) j["data"] = *data;
        return j;
    }
};

} // namespace mtype::lsp
