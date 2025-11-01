#pragma once

#include <optional>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class HoverHandler {
public:
    explicit HoverHandler(DocumentManager* docMgr);

    std::optional<Hover> handleHover(const std::string& uri, const Position& position);

private:
    std::optional<Hover> getKeywordHover(const std::string& word) const;
    std::optional<Hover> getTypeHover(const std::string& word) const;
    std::optional<Hover> getBuiltinHover(const std::string& word) const;

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
