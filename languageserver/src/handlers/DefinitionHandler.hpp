#pragma once

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"
#include <optional>

namespace mtype::lsp {

class DefinitionHandler {
public:
    explicit DefinitionHandler(DocumentManager* documentManager)
        : documentManager_(documentManager) {}

    // Handle textDocument/definition request
    std::optional<Location> handleDefinition(const std::string& uri, const Position& position);

private:
    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
