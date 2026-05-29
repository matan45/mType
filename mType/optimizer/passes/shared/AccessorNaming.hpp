#pragma once

// Single source of truth for Lombok-style accessor naming. Pure string ops
// with no AST/LSP dependencies so BOTH the compiler-side synthesis
// (LombokAstBuilders / LombokCodegen) and the LSP "generate accessors" code
// action (languageserver AccessorCodegen) derive method names identically —
// if the convention ever changes (e.g. boolean `is` getters), it changes in
// one place and the two stay byte-compatible.

#include <cctype>
#include <string>

namespace optimizer::passes::shared
{
    // "name" -> "Name"; empty string stays empty.
    inline std::string capitalize(const std::string& s)
    {
        if (s.empty()) return s;
        std::string out = s;
        out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
        return out;
    }

    inline std::string getterName(const std::string& field) { return "get" + capitalize(field); }
    inline std::string setterName(const std::string& field) { return "set" + capitalize(field); }
}
