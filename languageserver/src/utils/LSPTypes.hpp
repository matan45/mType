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

struct CompletionItem {
    std::string label;
    int kind; // CompletionItemKind
    std::optional<std::string> detail;
    std::optional<std::string> documentation;
    std::optional<std::string> insertText;

    json toJson() const {
        json j = {
            {"label", label},
            {"kind", kind}
        };
        if (detail) j["detail"] = *detail;
        if (documentation) j["documentation"] = *documentation;
        if (insertText) j["insertText"] = *insertText;
        return j;
    }
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

struct Diagnostic {
    Range range;
    int severity; // DiagnosticSeverity
    std::string message;
    std::optional<std::string> source;

    json toJson() const {
        json j = {
            {"range", range},
            {"severity", severity},
            {"message", message}
        };
        if (source) j["source"] = *source;
        return j;
    }
};

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

    json toJson() const {
        json j = {
            {"title", title}
        };
        if (kind) j["kind"] = *kind;
        if (edit) j["edit"] = edit->toJson();
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
