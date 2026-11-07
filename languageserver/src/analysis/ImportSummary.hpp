#pragma once

#include <string>
#include <unordered_set>

namespace mtype::lsp {

/**
 * What a document's imports bring into scope, as worked out by
 * ImportResolver::resolveImports (MYT-440).
 *
 * `typeNames` holds every type name the imports make visible, at any
 * depth: classes and interfaces declared in the imported files, the names
 * listed in selective imports, and their aliases (`import { A as B }`
 * makes both A and B visible, as in the compiler).
 *
 * `complete` is true only when every import, direct or nested, resolved
 * to a readable, parseable file. A library import (`import lib ...`) or a
 * regex fallback after a failed parse also leaves it false. Analyses that
 * must not report false positives (unresolved-type diagnostics) skip a
 * document whose summary is incomplete.
 */
struct ImportSummary {
    bool complete = false;
    std::unordered_set<std::string> typeNames;
};

} // namespace mtype::lsp
