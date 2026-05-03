#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace mtype::lsp::utils
{
    // ASCII-only lowercase. Used by completion / workspace-index sort
    // and dedup paths where we want case-insensitive ordering for
    // identifier-shaped strings. Pulled out of two different anonymous
    // namespaces (CompletionHandler.cpp + WorkspaceSymbolIndex.cpp) so
    // there's a single definition. Non-ASCII bytes are passed through
    // unchanged because identifiers are ASCII in mType.
    inline std::string toLowerAscii(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
        return text;
    }
}
