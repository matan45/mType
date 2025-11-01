#include "DefinitionHandler.hpp"

namespace mtype::lsp {

std::optional<Location> DefinitionHandler::handleDefinition(const std::string& uri, const Position& position) {
    auto symbolLoc = documentManager_->findDefinition(uri, position.line, position.character);

    if (!symbolLoc) {
        return std::nullopt;
    }

    Location location;
    location.uri = symbolLoc->uri;
    location.range.start.line = symbolLoc->line;
    location.range.start.character = symbolLoc->column;
    location.range.end.line = symbolLoc->line;
    location.range.end.character = symbolLoc->column;

    return location;
}

} // namespace mtype::lsp
